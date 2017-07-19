/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file sched_plq.hpp
 *  \brief Scheduling Strategies used by the PLQ Stage
 *  
 *  Different implementations of the scheduling strategies adopted by the
 *  emitter functionality of the PLQ stage of the Pane Farming pattern.
 *  
 */

/* ***************************************************************************
 *  
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License version 3 as
 *  published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *  License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *  
 ****************************************************************************
 */

/* 
 * Author: Gabriele Mencagli <mencagli@di.unipi.it>
 * April 2016
 */

#ifndef SCHED_H
#define SCHED_H

// include
#include <cmath>
#include <vector>
#include <limits>
#include <algorithm>
#include <ff/lb.hpp>
#include <ff/node.hpp>
#include <ff/utils.hpp>
#include <pid.hpp>
#include <tuple_t.hpp>
#include <general.hpp>

using namespace ff;
using namespace std;

/* 
 * This file provides the following classes:
 * -Sched: abstract class extended by any strategy;
 * -PB_RR_Sched: pane-based round robin strategy;
 * -TB_RR_Sched: tuple-based round robin scheduling strategy;
 * -Adaptive_Sched: adaptive scheduling with splitting strategy of large panes;
 * -Adaptive_Sched_PID: adaptive scheduling with splitting strategy of large panes and controlled by a PID.
 */

// struct of the PLQ monitoring snapshot
struct PLQ_Snapshot {
	ff_loadbalancer * const lb; // load balancer object
	unsigned long n_punc; // number of generated punctuations
	vector<size_t> tuples_to_w; // tuples_to_w[i] is the number of tuples scheduled to the i-th PLQ worker
	vector<ticks> time_ws; // vector of ticks for each PLQ worker (from the getsvcticks statistics)
	vector<size_t> tasks_ws; // vector of tasks for each PLQ worker (from the getnumtask statistics)
	vector<size_t> cm_panes_split; // splitting factors of the completed panes

	// constructor
	PLQ_Snapshot(ff_loadbalancer * const _lb): lb(_lb), n_punc(0), tuples_to_w(NUM_WORKER_THREADS, 0), time_ws(NUM_WORKER_THREADS, 0), tasks_ws(NUM_WORKER_THREADS, 0) {
		// set the fastflow stats
		const svector<ff_node*> &workers = lb->getWorkers();
		for(int i=0; i<NUM_WORKER_THREADS; i++) {
			time_ws[i] = workers[i]->getsvcticks();
			tasks_ws[i] = workers[i]->getnumtask();
		}
	}

	// destructor
	~PLQ_Snapshot() {}

	// method to reset the PLQ monitoring snapshot
	void reset() {
		n_punc = 0;
		const svector<ff_node*> &workers = lb->getWorkers();
		for(int i=0; i<NUM_WORKER_THREADS; i++) {
			tuples_to_w[i] = 0;
			time_ws[i] = workers[i]->getsvcticks();
			tasks_ws[i] = workers[i]->getnumtask();
		}
		cm_panes_split.clear();
	}
};

/*! 
 *  \class Sched
 *  
 *  \brief Abstract Class of Scheduling Strategies
 *  
 *  Abstract class that should be extended by any class implementing a
 *  scheduling strategy of the PLQ stage.
 *  
 *  This class is defined in \ref Pane_Farming/include/sched_plq.hpp
 */
class Sched {
protected:
	size_t nw_plq; // current number of PLQ workers utilized by the PLQ
	ff_loadbalancer * const lb; // load balancer object
	atomic<PLQ_Snapshot *> cs_snap; // atomic pointer to the current PLQ monitoring snapshot (used by the elastic controller)
	vector<size_t> p_instances; // p_instances[i] is the total number of partitions of the i-th pane
	vector<size_t> pane_size; // pane_size[i] is the total size (no. of tuples) of the i-th pane
	size_t last_w = 0; // id of the last PLQ worker used by the scheduling strategy
	size_t next_pane_id = 0; // identifier of the next pane to complete
	vector<bool> activatedWorker; // activatedWorker[i] = true means that the i-th PLQ worker is currently activated

	// constructor
	Sched(size_t _nw_plq, ff_loadbalancer * const _lb): nw_plq(_nw_plq), lb(_lb), cs_snap(new PLQ_Snapshot(_lb)), activatedWorker(NUM_WORKER_THREADS, false) {
		fill_n(activatedWorker.begin(), nw_plq, true); // the first nw_plq PLQ workers are initially utilized
	}

	// destructor
	~Sched() {
		delete cs_snap.load();
	}

	// method to get the next PLQ worker id in the round robin sequence
	inline size_t get_rr_w() {
		for(int i=last_w; i<NUM_WORKER_THREADS; i++) {
			if(activatedWorker[i]) {
				last_w = (i+1) % NUM_WORKER_THREADS;
				return i;
			}
		}
		for(int i=0; i<last_w; i++) {
			if(activatedWorker[i]) {
				last_w = (i+1) % NUM_WORKER_THREADS;
				return i;
			}
		}
		abort(); // at least one PLQ worker must always be activated!
	}

	// method to obtain the id of the least loaded PLQ worker (llw)
	inline size_t get_llw_w() {
		// the llw is the PLQ worker with the smallest input queue length
		bool found = false;
		size_t min = numeric_limits<size_t>::max();
		size_t llw_id;
		const svector<ff_node*> &workers = lb->getWorkers();
		size_t w = last_w;
		// for all the existing PLQ workers
		for(size_t i=0; i<NUM_WORKER_THREADS; i++) {
			// if PLQ worker w is activated, we inspect its current input queue length
			if(activatedWorker[w]) {
				found = true;
				FFBUFFER *q = workers[w]->get_in_buffer();
				size_t len = q->length();
				if(len < min) {
					min = len;
					llw_id = w;
				}
			}
			w = (w+1) % NUM_WORKER_THREADS;
		}
		if(!found) abort(); // at least one PLQ worker must always be activated!
		// to implement a fair selection policy
		last_w = (llw_id + 1) % NUM_WORKER_THREADS;
		return llw_id;
	}

	// method to broadcast a PUNCT message to all the activated PLQ workers
	inline void broadcast_punc_to_w(tuple_t* t) {
		std::vector<size_t> retry;
		for(int i=0; i<NUM_WORKER_THREADS; i++) {
			if(activatedWorker[i]) {
				if(!lb->ff_send_out_to(t, i)) retry.push_back(i);
			}
		}
		while(retry.size()) {
			if(lb->ff_send_out_to(t, retry.back())) retry.pop_back();
			else ticks_wait(TICKS2WAIT); // wait a short time interval
   		}
	}

	// method to multicast a NEW_INST message to all the PLQ workers whose id is in ws
	inline void multicast_newinst_to_w(vector<size_t> &ws, size_t pane_id, size_t count_instances) {
		// without destination PLQ workers we simply return without effects
		if(ws.size() == 0) return;
		// create the message
		tuple_t *t = new tuple_t();
		t->create_newinst(pane_id, count_instances, ws.size());
		std::vector<size_t> retry;
		for(auto const &w_id: ws) {
			if(!activatedWorker[w_id]) abort(); // the PLQ worker must be activated!
			if(!lb->ff_send_out_to(t, w_id)) retry.push_back(w_id);
		}
		while(retry.size()) {
			if(lb->ff_send_out_to(t, retry.back())) retry.pop_back();
			else ticks_wait(TICKS2WAIT); // wait a short time interval
   		}
	}

	// method to multicast a FLUSH message to all the PLQ workers whose id is in ws
	inline void multicast_flush_to_w(vector<size_t> &ws, size_t pane_id) {
		// without destination PLQ workers we simply return without effects
		if(ws.size() == 0) return;
		// create the message
		tuple_t *t = new tuple_t();
		t->create_flush(ws.size(), pane_id);
		std::vector<size_t> retry;
		for(auto const &w_id: ws) {
			if(activatedWorker[w_id]) abort(); // the PLQ worker must be de-activated!
			if(!lb->ff_send_out_to(t, w_id)) retry.push_back(w_id);
		}
		while(retry.size()) {
			if(lb->ff_send_out_to(t, retry.back())) retry.pop_back();
			else ticks_wait(TICKS2WAIT); // wait a short time interval
   		}
	}

	/* 
	 * Function to compute the utilization factor of the PLQ stage. It is used
	 * by the elastic controller and by the internal PID of the PLQ emitter in case
	 * of the AS_PID scheduling strategy.
	 */
	double computePLQRho(size_t INTERVAL_USECS, PLQ_Snapshot *s) {
		int max_nw_plq = NUM_WORKER_THREADS;
		// statistics of the last time interval
		double lambdas[max_nw_plq]; // lambdas[i] is the amount of tuples/punctuations sent to the i-th PLQ worker
		double mus[max_nw_plq]; // mus[i] is the ideal number of tuples/punctuations that the i-th PLQ worker can process
		double time_idle[max_nw_plq]; // time_idle[i] is the length (usec) of the fraction of the last time interval passed by the i-th PLQ worker in idle mode
		double lambda = 0; // total amount of tuples/punctuations transmitted by the PLQ emitter
		double avg_ts = 0; // estimated average processing time per tuple/punctuation (usec)
		double tot_time_working = 0; // sum of the time intervals spent by all the PLQ workers in processing tuples/punctuations
		double tot_tasks = 0; // total number of tuples/punctuations processed by all the PLQ workers
		double rho = 0; // computed utilization factor of the PLQ stage
		// compute lambdas and lambda
		for(size_t i=0; i<max_nw_plq; i++) {
			lambdas[i] = s->tuples_to_w[i] + s->n_punc;
			lambda += s->tuples_to_w[i] + s->n_punc;
		}
		// get the pointers to the PLQ workers
		const svector<ff_node*> &workers = lb->getWorkers();
		for(size_t i=0; i<max_nw_plq; i++) {
			// total time (in ticks) passed in the svc by the i-th PLQ worker from the beginning of the execution
			ticks active_time_ticks = workers[i]->getsvcticks();
			// time passed in processing tuples/punctuations by the i-th PLQ worker during the last time interval
			double time_working = FROM_TICKS_TO_USECS(active_time_ticks - s->time_ws[i]);
			tot_time_working += time_working;
			// idle time of the i-th PLQ worker
			time_idle[i] = std::max(0.0, INTERVAL_USECS - time_working);
			// total number of tuples/punctuations processed by the i-th PLQ worker from the beginning of the execution
			int tasks = workers[i]->getnumtask();
			// number of tuples/punctuations processed by the i-th PLQ worker during the last time interval
			mus[i] = tasks - s->tasks_ws[i];
			tot_tasks += mus[i];
		}
		// compute the estimated average processing time per tuple
		avg_ts = tot_time_working / tot_tasks;
		// compute the utilization factor of the PLQ by considering only the utilized PLQ workers
		for(size_t i=0; i<max_nw_plq; i++) {
			mus[i] += time_idle[i] / avg_ts;
			if(activatedWorker[i]) rho += (lambdas[i] * lambdas[i]) / (lambda * mus[i]);
		}
		if(std::isnan(rho) || std::isinf(rho)) return 0;
		else return rho;
	}

public:
	// method to schedule a new tuple to a PLQ worker
	virtual void sched_to_worker(tuple_t *t) = 0;

	// method to start utilizing additional num>0 PLQ workers with given identifiers
	void addWorkers(size_t num, vector<size_t>& w_ids) {
		nw_plq = nw_plq + num;
		for(auto const& w: w_ids) {
			if(activatedWorker[w]) abort(); // the PLQ worker must be previously de-activated!
			activatedWorker[w] = true;
			lb->thaw(w, true);	
		}
	}

	// method to stop utilizing num>0 PLQ workers with given identifiers
	virtual void removeWorkers(size_t num, vector<size_t>& w_ids) = 0;

	// method to manage the stream progress
	inline void executeStreamProgress(double ts, size_t p_id) {
		/* 
		 * All the panes with id < p_id are complete. We do the following actions:
		 * 1- we check the presence of a complete pane with zero tuples;
		 * 2- We send a lull message to an activated PLQ worker in order to produce the result of the lull pane;
		 * 3- we send a punctuation with value ts to all the activated PLQ workers (only OOP).
		 */
		if(p_id < next_pane_id) return;
		else {
			if(p_id >= p_instances.size()) {
				p_instances.resize(p_id+1, 0);
				pane_size.resize(p_id+1, 0);
			}
		}
		// check for lulls (empty panes that will remain empty)
		for(int i=next_pane_id; i<p_id; i++) {
			if(pane_size[i] == 0) {
				p_instances[i] = 1; // the pane has one partition
				// emit a special tuple indicating a lull
				tuple_t *t = new tuple_t();
				t->create_lull(i);
				size_t w = get_rr_w();
				while(!lb->ff_send_out_to(t, w));
			}
			// the i-th pane has been completed during the current snapshot
			((cs_snap.load())->cm_panes_split).push_back(p_instances[i]);
		}
		next_pane_id = p_id;
#if !defined(IOP)
		// update the number of puntuations generated
		(cs_snap.load())->n_punc++;
		// create the punctuation
		tuple_t *p = new tuple_t();
		p->create_punctuation(ts, p_id, nw_plq);
		// broadcast the punctuation to all the activated PLQ workers
		broadcast_punc_to_w(p);
#endif
	}

	// method to acquire the PLQ monitored data
	PLQ_Monitored_Data getMonitoredData() {
		// create a new monitoring snapshot that will be atomically swapped with the old one
		PLQ_Snapshot *old_snap = cs_snap.exchange(new PLQ_Snapshot(lb));
		// compute the utilization factor
		double rho = computePLQRho(SAMPLING_ASPID_PLQ_USEC, old_snap);
		// return also the number of partitions of the panes completed during the snapshot
		vector<size_t> split_vect = old_snap->cm_panes_split;
		delete old_snap;
		return PLQ_Monitored_Data(rho, split_vect);
	}

	// method to get the average splitting factor
	double getPLQSplitFactor() const {
		double avg = 0;
		for(auto const &c: p_instances) avg += c;
		avg = avg / p_instances.size();
		return avg;
	}

	// method to send a END to all the PLQ workers (also the currently de-activated ones)
	void broadcast_end_to_w() {
		// wake up all the de-activated PLQ workers
		for(int i=0; i<NUM_WORKER_THREADS; i++) {
			if(!activatedWorker[i]) {
				activatedWorker[i] = true;
				lb->thaw(i, true);
			}
		}
		// create the END message
		tuple_t *end = new tuple_t();
		end->create_end(NUM_WORKER_THREADS);
		// broadcast the END message to all the PLQ workers
		lb->broadcast_task(end);
	}
};

/*! 
 *  \class PB_RR_Sched
 *  
 *  \brief Pane-based Round-Robin Scheduling Strategy
 *  
 *  Strategy that assigns each new pane to a PLQ worker in a round-robin
 *  fashion.
 *  
 *  This class is defined in \ref Pane_Farming/include/sched_plq.hpp
 */
class PB_RR_Sched: public Sched {
private:
	vector<int> panes_ws; // panes_ws[i] is the id of the PLQ worker assigned to the i-th pane (-1 not assigned yet)

public:
	// constructor
	PB_RR_Sched(size_t _nw_plq, ff_loadbalancer * const _lb): Sched(_nw_plq, _lb) {}

	// destructor
	~PB_RR_Sched() {}

	// method to schedule a new tuple to a PLQ worker
	inline void sched_to_worker(tuple_t *t) {
		// get the id of the pane
		size_t pane_id = t->pane_id;
		if(pane_id >= p_instances.size()) {
			p_instances.resize(pane_id+1, 0);
			pane_size.resize(pane_id+1, 0);
			panes_ws.resize(pane_id+1, -1);
		}
		// increase the counter of the number of tuples per pane
		pane_size[pane_id]++;
		// if the pane has not been assigned to a PLQ worker yet
		if(panes_ws[pane_id] == -1) {
			// select the next PLQ worker using a RR policy
			size_t w = get_rr_w();
			panes_ws[pane_id] = w;
			p_instances[pane_id] = 1;
			(cs_snap.load())->tuples_to_w[w]++;
			t->no_instances = p_instances[pane_id];
			while(!lb->ff_send_out_to(t, w));
		}
		// the pane already has an assigned PLQ worker
		else {
			size_t w = panes_ws[pane_id];
			(cs_snap.load())->tuples_to_w[w]++;
			t->no_instances = p_instances[pane_id];
			while(!lb->ff_send_out_to(t, w));
		}
	}

	// method to stop utilizing num>0 PLQ workers with given identifiers
	void removeWorkers(size_t num, vector<size_t>& w_ids) {
		nw_plq = nw_plq - num;
		for(auto const& w: w_ids) {
			// the w-th PLQ worker must be de-activated!
			if(!activatedWorker[w]) abort(); // the PLQ worker must be previously activated!
			activatedWorker[w] = false;
		}
		/* 
		 * We send a FLUSH message to all the deactivated PLQ workers and we
         * de-activate them.
         */
		multicast_flush_to_w(w_ids, next_pane_id);
		for(auto const& w: w_ids) {
			lb->ff_send_out_to(GO_OUT, w); // we de-activate the w-th PLQ worker
		}
		/* 
		 * For all the non-closed panes p we check:
		 * 1- if p is held by a de-activated PLQ worker, we send an EMPTYPANE message
		 *    for pane p to a still activated PLQ worker;
		 * 2- otherwise, we do nothing.
		 */
		for(int p=next_pane_id; p<panes_ws.size(); p++) {
			if(find(w_ids.begin(), w_ids.end(), panes_ws[p]) != w_ids.end()) {
				// send an EMPTYPANE message for the pane p to a still activated PLQ worker
				p_instances[p]++; // we have another partition of that pane
				tuple_t *t = new tuple_t();
				t->create_emptypane(p, p_instances[p]);
				size_t w = get_rr_w();
				panes_ws[p] = w;
				while(!lb->ff_send_out_to(t, w));
			}
		}
	}

	// method to manage the stream progress
	inline void executeStreamProgress(double ts, size_t p_id) {
		if(p_id < next_pane_id) return;
		else if(p_id >= panes_ws.size()) panes_ws.resize(p_id+1, -1);
		return Sched::executeStreamProgress(ts, p_id);
	}
};

/*! 
 *  \class TB_RR_Sched
 *  
 *  \brief Tuple-based Round-Robin Scheduling Strategy
 *  
 *  Strategy that assigns each new tuple to a PLQ worker selected in a
 *  round-robin fashion.
 *  
 *  This class is defined in \ref Pane_Farming/include/sched_plq.hpp
 */
class TB_RR_Sched: public Sched {
private:
	vector<vector<size_t>> p_workers; // p_worker[i] is a vector containing the ids of the PLQ workers having a partition of the i-th pane

public:
	// constructor
	TB_RR_Sched(size_t _nw_plq, ff_loadbalancer * const _lb): Sched(_nw_plq, _lb) {}

	// destructor
	~TB_RR_Sched() {}

	// method to schedule a new tuple to a PLQ worker
	inline void sched_to_worker(tuple_t *t) {
		// get the id of the pane
		size_t pane_id = t->pane_id;
		if(pane_id >= p_instances.size()) {
			p_instances.resize(pane_id+1, 0);
			pane_size.resize(pane_id+1, 0);
			p_workers.resize(pane_id+1);
		}
		// increase the counter of the number of tuples per pane
		pane_size[pane_id]++;
		// get the id of the next PLQ worker in a round-robin fashion
		size_t w = get_rr_w();	
		// if the selected PLQ worker does not still have a partition of pane_id
		if(find(p_workers[pane_id].begin(), p_workers[pane_id].end(), w) == p_workers[pane_id].end()) {
			// multicast the presence of a new partition to the interested PLQ workers
			p_instances[pane_id]++;
			multicast_newinst_to_w(p_workers[pane_id], pane_id, p_instances[pane_id]);
			// add the PLQ worker to the list of PLQ workers having a partition of pane_id
			p_workers[pane_id].push_back(w);
		}
		// send the tuple to the selected PLQ worker
		(cs_snap.load())->tuples_to_w[w]++;
		t->no_instances = p_instances[pane_id];
		while(!lb->ff_send_out_to(t, w));
	}

	// method to stop utilizing num>0 PLQ workers with given identifiers
	void removeWorkers(size_t num, vector<size_t>& w_ids) {
		nw_plq = nw_plq - num;
		for(auto const& w: w_ids) {
			if(!activatedWorker[w]) abort(); // the PLQ worker must be previously activated!
			activatedWorker[w] = false;
		}
		/* 
		 * We send a FLUSH message to all the PLQ workers running on cpu_ids and we
         * de-activate them.
         */
		multicast_flush_to_w(w_ids, next_pane_id);
		for(auto const& w: w_ids) lb->ff_send_out_to(GO_OUT, w); // we de-activate the w-th PLQ worker
		/* 
		 * For all the non-closed panes p we check:
		 * 1- if there do not exist any activated PLQ workers with a partition of pane p, we send an
		 *    EMPTYPANE message for that pane to a still activated PLQ worker;
		 * 2- otherwise, we do nothing.
		 */
		for(int p=next_pane_id; p<p_workers.size(); p++) {
			bool found = false;
			vector<size_t> new_worker_list;
			// for each PLQ worker w having a partition of pane p
			for(auto const& w: p_workers[p]) {
				if(activatedWorker[w]) {
					found = true;
					new_worker_list.push_back(w);
				}
			}
			// if there does not exist a PLQ worker with a partition of pane p
			if(!found) {
				// send an EMPTYPANE message for the pane p to a still activated PLQ worker
				p_instances[p]++; // we have another partition of that pane
				tuple_t *t = new tuple_t();
				t->create_emptypane(p, p_instances[p]);
				size_t w = get_rr_w();
				new_worker_list.push_back(w);
				while(!lb->ff_send_out_to(t, w));
			}
			// update the new PLQ worker list of the pane p
			p_workers[p] = new_worker_list;
		}
	}

	// method to manage the stream progress
	inline void executeStreamProgress(double ts, size_t p_id) {
		if(p_id < next_pane_id) return;
		else if(p_id >= p_workers.size()) p_workers.resize(p_id+1);
		return Sched::executeStreamProgress(ts, p_id);
	}
};

/*! 
 *  \class Adaptive_Sched
 *  
 *  \brief Adaptive Scheduling Strategy
 *  
 *  First implementation of the adaptive scheduling strategy used by the PLQ
 *  stage of the Pane Farming pattern. If the actual pane partition size is
 *  greater than a historical measure, we split a pane into another partition
 *  assigned to a distinct PLQ worker. The goal is to split only large panes.
 *  
 *  This class is defined in \ref Pane_Farming/include/sched_plq.hpp
 */
class Adaptive_Sched: public Sched {
protected:
	// Pane Descriptor: struct with useful information of a pane
	struct pane_des_t {
		int a_worker; // id of the PLQ worker assigned to the active partition of the pane
		vector<size_t> w_ids; // ids of the PLQ workers that have a partition of the pane
		size_t a_tuples; // number of tuples of the active partition of the pane
		vector<size_t> i_sizes; // i_sizes[i] is the size of the partition of the i-th PLQ worker
		// default constructor
		pane_des_t(): i_sizes(NUM_WORKER_THREADS, 0) { a_worker = -1; a_tuples = 0; }
	};
	vector<pane_des_t> pv; // vector of pane descriptors
	double avg_size = 0; // average size of the pane partitions
	double Sn_size = 0; // n*\sigma_{n}^{2} of the pane partitions size
	double std_size = 0; // standard deviation of the pane partitions size
	double tot_instances = 0; // number of pane partitions produced
	bool warmup = true; // true if we are in the initial warmup period in order to have good estimates of avg_size and std_size

public:
	// constructor
	Adaptive_Sched(size_t _nw_plq, ff_loadbalancer * const _lb): Sched(_nw_plq, _lb) {}

	// destructor
	~Adaptive_Sched() {}

	// method to schedule a new tuple to a PLQ worker
	inline void sched_to_worker(tuple_t *t) {
		// create the pane descriptor if it does not exist
		size_t pane_id = t->pane_id;
		if(pane_id >= p_instances.size()) {
			p_instances.resize(pane_id+1, 0);
			pane_size.resize(pane_id+1, 0);
			pv.resize(pane_id+1);
		}
		// increase the counter of the number of tuples per pane
		pane_size[pane_id]++;
		// Case 1: the tuple is the first of a new pane
		if(pane_size[pane_id] == 1) {
			// we schedule to the least loaded PLQ worker
			int w = get_llw_w();
			(cs_snap.load())->tuples_to_w[w]++;
			pv[pane_id].a_tuples = 1;
			pv[pane_id].a_worker = w;
			p_instances[pane_id] = 1;
			pv[pane_id].w_ids.push_back(w);
			(pv[pane_id].i_sizes)[w] = 1;
			t->no_instances = p_instances[pane_id];
			// send the tuple to the selected PLQ worker
			while(!lb->ff_send_out_to(t, w));
		}
		// Case 2: the tuple belongs to an existing pane
		else {
			// if the pane istance is not so large or we are in the warmup period
			if((pv[pane_id].a_tuples <= (avg_size + std_size)) || warmup) {
				// schedule the tuple as before
				int w = pv[pane_id].a_worker;
				(cs_snap.load())->tuples_to_w[w]++;
				pv[pane_id].a_tuples++;
				(pv[pane_id].i_sizes)[w]++;
				t->no_instances = p_instances[pane_id];
				while(!lb->ff_send_out_to(t, w));
			}
			// otherwise we create another partition of the pane
			else {
				// we find the least loaded PLQ worker
				int w = get_llw_w();
				// if this PLQ worker already has a partition, it becomes the active one again
				vector<size_t> &w_ids = pv[pane_id].w_ids;
				if(find(w_ids.begin(), w_ids.end(), w) != w_ids.end()) {
					(cs_snap.load())->tuples_to_w[w]++;
					pv[pane_id].a_tuples = (pv[pane_id].i_sizes)[w] + 1;
					pv[pane_id].a_worker = w;
					(pv[pane_id].i_sizes)[w]++;
					t->no_instances = p_instances[pane_id];
					while(!lb->ff_send_out_to(t, w));
				}
				// else we create a new partition of the pane that becomes the active one
				else {
					(cs_snap.load())->tuples_to_w[w]++;
					pv[pane_id].a_tuples = 1;
					pv[pane_id].a_worker = w;
					p_instances[pane_id]++;
					(pv[pane_id].i_sizes)[w] = 1;
					// multicast the presence of a new partition to the interested PLQ workers
					multicast_newinst_to_w(pv[pane_id].w_ids, pane_id, p_instances[pane_id]);
					pv[pane_id].w_ids.push_back(w);
					t->no_instances = p_instances[pane_id];
					// send the tuple to the new selected PLQ worker
					while(!lb->ff_send_out_to(t, w));
				}
			}
		}
	}

	// method to stop utilizing num>0 PLQ workers with given identifiers
	void removeWorkers(size_t num, vector<size_t>& w_ids) {
		nw_plq = nw_plq - num;
		for(auto const& w: w_ids) {
			if(!activatedWorker[w]) abort(); // the PLQ worker must be previously activated!
			activatedWorker[w] = false;
		}
		// we send a FLUSH message to all the PLQ workers to deactivate
		multicast_flush_to_w(w_ids, next_pane_id);
		// we de-activate all the PLQ workers whose id is in w_ids
		for(auto const& w: w_ids) lb->ff_send_out_to(GO_OUT, w);
		/* 
		 * For all the non-closed panes p we check:
		 * 1- if there do not exist any activated PLQ workers with a partition of pane p, we send an
		 *    EMPTYPANE message for that pane to a still activated PLQ worker;
		 * 2- otherwise, we do nothing.
		 */
		for(int p=next_pane_id; p<pv.size(); p++) {
			bool found = false;
			vector<size_t> new_worker_list;
			// for each PLQ worker w having a partition of pane p
			for(auto const& w: pv[p].w_ids) {
				if(activatedWorker[w]) { // if the PLQ worker is still activated
					found = true;
					new_worker_list.push_back(w);
				}
				else { // we reset the counter of tuples of that PLQ worker
					(pv[p].i_sizes)[w] = 0;
				}
			}
			// update the new PLQ worker list of the pane p
			pv[p].w_ids = new_worker_list;
			/* 
			 * We force a pane splitting in two cases:
			 * 1- no still activated PLQ worker has a partition of pane p;
			 * 2- the active PLQ worker for pane p is not activated anymore.
			 */
			if(!found || (!activatedWorker[pv[p].a_worker])) {
				// we pick the identifier of the llw
				size_t w = get_llw_w();
				// if the PLQ worker has already a partition of pane p
				if(find(pv[p].w_ids.begin(), pv[p].w_ids.end(), w) != pv[p].w_ids.end()) {
					pv[p].a_worker = w;
					pv[p].a_tuples = (pv[p].i_sizes)[p];
				}
				else {
					// we force the creation of a new partition of pane p
					p_instances[p]++;
					pv[p].a_worker = w;
					pv[p].a_tuples = 0;
					(pv[p].i_sizes)[w] = 0;
					// multicast the presence of a new partition to the interested PLQ workers
					multicast_newinst_to_w(pv[p].w_ids, p, p_instances[p]);
					pv[p].w_ids.push_back(w);
					// send an EMPTYPANE message for the pane p to w
					tuple_t *t = new tuple_t();
					t->create_emptypane(p, p_instances[p]);
					// send the tuple to the new selected PLQ worker
					while(!lb->ff_send_out_to(t, w));
				}
			}
		}
	}

	// method to manage the stream progress
	inline void executeStreamProgress(double ts, size_t p_id) {
		/* 
		 * All the panes with id < p_id are complete. We do the following actions:
		 * 1- we check the presence of a complete pane with zero tuples;
		 * 2- We send a lull message to an activated PLQ worker in order to produce the result of the empty pane;
		 * 3- we send a punctuation with value ts to all the activated PLQ workers (only OOP).
		 */
		if(p_id < next_pane_id) return;
		else {
			if(p_id >= p_instances.size()) {
				p_instances.resize(p_id+1, 0);
				pane_size.resize(p_id+1, 0);
				pv.resize(p_id+1);
			}
		}
		/* For all the completed panes we check:
		 * 1- if the pane has no tuples, we emit a lull message to a still activated PLQ worker;
		 * 2- we update the statistics of the completed panes (avg size and std).
		 */
		for(int i=next_pane_id; i<p_id; i++) {
			if(pane_size[i] == 0) {
				p_instances[i] = 1; // the pane has one partition
				// emit a special tuple indicating a lull
				tuple_t *t = new tuple_t();
				t->create_lull(i);
				size_t w = get_rr_w();
				while(!lb->ff_send_out_to(t, w));
			}
			for(auto const& s: pv[i].i_sizes) { // for all the partitions of the pane
				if(s > 0) {
					tot_instances++;
					double old_avg_size = avg_size;
					avg_size += (((double) 1)/tot_instances) * (s - old_avg_size);
					Sn_size += (s - old_avg_size) * (s - avg_size);
				}
			}
			// the i-th pane has been completed during the current snapshot
			((cs_snap.load())->cm_panes_split).push_back(p_instances[i]);
		}
		std_size= sqrt(((double) Sn_size)/tot_instances);
		// we conclude the warmup period after the completion of the first five pane (heuristic)
		if(p_id > 5) warmup = false;
		next_pane_id = p_id;
#if !defined(IOP)
		(cs_snap.load())->n_punc++;
		// create the punctuation
		tuple_t *p = new tuple_t();
		p->create_punctuation(ts, p_id, nw_plq);
		// broadcast the punctuation to all the activated PLQ workers
		broadcast_punc_to_w(p);
#endif
	}
};

/*! 
 *  \class Adaptive_Sched_PID
 *  
 *  \brief Adaptive Scheduling Strategy with PID-based Control
 *  
 *  Second implementation of the adaptive scheduling strategy used by the PLQ
 *  stage of the Pane Farming pattern. This implementation is an extension of
 *  Adaptive_Sched in which we dynamically adapt the threshold value used to
 *  determine whether a pane partition must be divided or not.
 *  
 *  This class is defined in \ref Pane_Farming/include/sched_plq.hpp
 */
class Adaptive_Sched_PID: public Adaptive_Sched {
private:
	PID *pid; // PID controller used to dynamically adapt the threshold value
	double alpha = 1; // corrective factor of the threshold used to split the pane partitions
	PLQ_Snapshot pid_snap; // PLQ monitoring snapshot used by the internal PID
	volatile ticks last_PID_sample_time; // ticks of the starting time of the last internal sampling of the PID

public:
	// constructor
	Adaptive_Sched_PID(size_t _nw_plq, ff_loadbalancer * const _lb, ticks _starting_time, double _desired_rho): Adaptive_Sched(_nw_plq, _lb), pid_snap(_lb), last_PID_sample_time(_starting_time) {
		// create the controller
		double Kp = 1.8; double Ki = 0; double Kd = 2; // <----------(ad hoc values - to be checked)
		pid = new PID(Kp, Ki, Kd, _desired_rho, -0.3, 0.3, true, true /* isReversed=true */); // desired_rho is a global variable
	}

	// destructor
	~Adaptive_Sched_PID() {
		delete pid;
	}

	// method to schedule a new tuple to a PLQ worker
	inline void sched_to_worker(tuple_t *t) {
		// check the internal sampling of the PID
		double elapsed = FROM_TICKS_TO_USECS(getticks() - last_PID_sample_time);
		if(elapsed >= SAMPLING_ASPID_PLQ_USEC) {
			last_PID_sample_time = getticks();
			// get the utilization factor
			double rho = computePLQRho(SAMPLING_ASPID_PLQ_USEC, &pid_snap);
			pid_snap.reset(); // reset the PID snapshot
 			// compute the PID output to adapt the threshold
			pid->setInput(rho);
			if(!pid->process()) cout << "PID is disabled" << endl;
			double output = pid->getOutput();
			// add to the value of alpha the PID output
			alpha += output;
			if(alpha < 0.1) alpha = 0.1; // check the minimum value of alpha
			else if(alpha > 20) alpha = 20; // check the maximum value of alpha
			//cout << "PLQ rho: " << rho << " new alpha: " << alpha << " new threshold: " << (1/alpha) * (avg_size + std_size) << endl;
		}
		// create the pane descriptor if it does not exist
		size_t pane_id = t->pane_id;
		if(pane_id >= p_instances.size()) {
			p_instances.resize(pane_id+1, 0);
			pane_size.resize(pane_id+1, 0);
			pv.resize(pane_id+1);
		}
		// increase the counter of the number of tuples per pane
		pane_size[pane_id]++;
		// Case 1: the tuple is the first of a new pane
		if(pane_size[pane_id] == 1) {
			// we schedule to the least loaded PLQ worker
			int w = get_llw_w();
			pid_snap.tuples_to_w[w]++;
			(cs_snap.load())->tuples_to_w[w]++;
			pv[pane_id].a_tuples = 1;
			pv[pane_id].a_worker = w;
			p_instances[pane_id] = 1;
			pv[pane_id].w_ids.push_back(w);
			(pv[pane_id].i_sizes)[w] = 1;
			t->no_instances = p_instances[pane_id];
			// send the tuple to the selected PLQ worker
			while(!lb->ff_send_out_to(t, w));
		}
		// Case 2: the tuple belongs to an existing pane
		else {
			// compute the actual threshold
			double threshold = (1/alpha) * (avg_size + std_size);
			// if the active pane partition is not large enough or we are in the warmup period
			if((pv[pane_id].a_tuples <= threshold) || warmup) {
				// schedule the tuple as before
				int w = pv[pane_id].a_worker;
				pid_snap.tuples_to_w[w]++;
				(cs_snap.load())->tuples_to_w[w]++;
				pv[pane_id].a_tuples++;
				(pv[pane_id].i_sizes)[w]++;
				t->no_instances = p_instances[pane_id];
				while(!lb->ff_send_out_to(t, w));
			}
			// otherwise we create another partition of the pane
			else {
				// we find the least loaded PLQ worker
				int w = get_llw_w();
				// if this PLQ worker already has a partition, it becomes the active one again
				vector<size_t> &w_ids = pv[pane_id].w_ids;
				if(find(w_ids.begin(), w_ids.end(), w) != w_ids.end()) {
					pid_snap.tuples_to_w[w]++;
					(cs_snap.load())->tuples_to_w[w]++;
					pv[pane_id].a_tuples = (pv[pane_id].i_sizes)[w] + 1;
					pv[pane_id].a_worker = w;
					(pv[pane_id].i_sizes)[w]++;
					t->no_instances = p_instances[pane_id];
					while(!lb->ff_send_out_to(t, w));
				}
				// else we create a new partition of the pane that becomes the active one
				else {
					pid_snap.tuples_to_w[w]++;
					(cs_snap.load())->tuples_to_w[w]++;
					pv[pane_id].a_tuples = 1;
					pv[pane_id].a_worker = w;
					p_instances[pane_id]++;
					(pv[pane_id].i_sizes)[w] = 1;
					// multicast the presence of a new partition to the interested PLQ workers
					multicast_newinst_to_w(pv[pane_id].w_ids, pane_id, p_instances[pane_id]);
					pv[pane_id].w_ids.push_back(w);
					t->no_instances = p_instances[pane_id];
					// send the tuple to the new selected PLQ worker
					while(!lb->ff_send_out_to(t, w));
				}
			}
		}
	}

	// method to manage the stream progress
	inline void executeStreamProgress(double ts, size_t p_id) {
#if !defined(IOP)
		pid_snap.n_punc++;
#endif
		return Adaptive_Sched::executeStreamProgress(ts, p_id);
	}
};

#endif
