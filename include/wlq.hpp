/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file wlq.hpp
 *  \brief Window-level Query Stage
 *  
 *  Implementation of the Window-level Query (WLQ) stage, which is the
 *  second stage of the Pane Farming pattern. WLQ is a farm where the
 *  emitter is responsible to schedule WLQ tasks to WLQ workers that
 *  process them. When the results set of a window is complete, it is
 *  forwared to the WLQ collector that emits window results sets ordered
 *  by the window identifier.
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
 * May 2016
 */

#ifndef WLQ_H
#define WLQ_H

// include
#include <deque>
#include <algorithm>
#include <ff/lb.hpp>
#include <ff/node.hpp>
#include <ff/farm.hpp>
#include <ff/mapping_utils.hpp>
#include <general.hpp>
#include <queries.hpp>

using namespace ff;
using namespace std;

/* 
 * This file provides the following classes:
 * -WLQ_Worker: class that implements the worker of the WLQ stage;
 * -WLQ_Emitter: class that implements the emitter of the WLQ stage;
 * -WLQ_Collector: class that implements the collector of the WLQ stage.
 */

/* 
 * Enumeration of the possible types of WLQ tasks:
 * -WIN_UPDATE = task to update the results set of a window with another results set;
 * -MERGE = task to merge two results sets and produce a third one.
 */
enum type_wtask_t { WIN_UPDATE, MERGE };

// struct of a WLQ task scheduled by the WLQ emitter to the WLQ workers
template <typename Rset_t>
struct WLQ_Task_t {
	type_wtask_t type; // type of the WLQ task
	Rset_t *p1; // first results set
	Rset_t *p2; // second results set
	size_t win_id; // window id related to this task (starting from 0)
	int worker_id = -1; // worker id (starting from 0) assigned to this task (-1 not specified yet)
	/* 
	 * If finalize is true, the first results set p1 must be transmitted
	 * to the collector (finalized) after the completion of this task.
	 */
	bool finalize; // meaningful only if type == WIN_UPDATE

	// constructor
	WLQ_Task_t(type_wtask_t _type, Rset_t *_p1, Rset_t *_p2, size_t _win_id, bool _finalize=false): type(_type), p1(_p1), p2(_p2), win_id(_win_id), finalize(_finalize) {}
};

// struct of the WLQ monitoring snapshot
struct WLQ_Snapshot {
	vector<size_t> tasks_to_w; // tasks_to_w[i] is the number of WLQ tasks scheduled to the i-th WLQ worker
	size_t cnt_win_tasks; // total number of WIN_UPDATE WLQ tasks scheduled to the WLQ workers
	size_t cnt_merge_tasks; // total number of MERGE WLQ tasks scheduled to the WLQ workers

	// constructor
	WLQ_Snapshot(): tasks_to_w(NUM_WORKER_THREADS, 0), cnt_win_tasks(0), cnt_merge_tasks(0) {}

	// destructor
	~WLQ_Snapshot() {}
};

/*! 
 *  \class WLQ_Worker
 *  
 *  \brief Worker of the Window-level Query Stage
 *  
 *  Worker functionality of the WLQ stage of the Pane Farming. It is responsible to
 *  receive WLQ tasks and to execute them by eventually forwarding the results
 *  sets of the completed windows to the WLQ collector.
 *  
 *  This class is defined in \ref Pane_Farming/include/wlq.hpp
 */
template<typename Rset_t>
class WLQ_Worker: public ff_monode_t<WLQ_Task_t<Rset_t>, Rset_t> {
private:
	typedef WLQ_Task_t<Rset_t> w_task_t;
	size_t rcv_wtasks = 0; // number of processed WLQ tasks by this WLQ worker
	size_t rcv_win_finalized = 0; // number of finalized windows
	double avg_wtask_time = 0; // average time to process a WLQ task (usec)
	double avg_win_results = 0; // average number of result tuples per finalized window
	volatile ticks time_elapsed_svc = 0; // global ticks passed in the svc
	int cpuid; // identifier of the SMT context on which this WLQ worker is executed
	bool flagCPUInitialized = false; // if this flag is false, we need to initialize the SMT context of this WLQ worker (just the first time)

public:
	// constructor
	WLQ_Worker(int _cpuid): cpuid(_cpuid) {}

	// destructor
	~WLQ_Worker() {}

	// svc_init method
	int svc_init() {
		// set the WLQ worker thread on the given SMT context if needed
		if(!flagCPUInitialized) {
			ff_mapThreadToCpu(cpuid);
			flagCPUInitialized = true;
		}
		return 0;
	}

	// svc_end method
	void svc_end() {}

	// method to properly manage the arrival of the EOS
	void eosnotify(ssize_t id) {
		// if LOG mode is enabled we write on the log file
#if defined(LOG)
		ostringstream stream;
 		stream << "****************WLQ worker " << this->get_my_id() << "****************\n";
 		stream << "Received WLQ tasks: " << rcv_wtasks << "\n";
 		stream << "Average no. of tuples in the results set of a window: " << avg_win_results << "\n";
 		stream << "Average processing time per WLQ task (us): " << avg_wtask_time << "\n";
    	LOG_WLQ_WRITE(stream);
#endif
	}

	// svc method
	Rset_t *svc(w_task_t *w_task) {
		// get the initial time of the svc
		volatile ticks start_ticks = getticks();
		Rset_t *p1 = w_task->p1; // first results set
		Rset_t *p2 = w_task->p2; // second results set
		// if the task type is WIN_UPDATE
		if(w_task->type == WIN_UPDATE) {
			// update the results set of the window (p1) with the results set of a pane partition (p2)
			p1->addComputeSet(*p2);
			// deallocate p2 if it is not needed by any other WLQ task
			if(p2->decreaseRefCounter() == 1) delete p2;
			w_task->p2 = nullptr;
		}
		// if the task type is MERGE
		else if(w_task->type == MERGE) {
			// create a new results set p3
			Rset_t *p3 = new Rset_t(1, 0, 1); // id 0 non meaningful
			p3->addComputeSet(*p1);
			p3->addComputeSet(*p2);
			// deallocate p1 and/or p2 if they are not needed by any other WLQ task
			if(p1->decreaseRefCounter() == 1) delete p1;
			if(p2->decreaseRefCounter() == 1) delete p2;
			w_task->p1 = p3;
			w_task->p2 = nullptr;
		}
		// get the final time of the svc
		volatile ticks end_ticks = getticks();
		double elapsed = FROM_TICKS_TO_USECS(end_ticks - start_ticks); // processing time of the WLQ task
		// update statistics
		rcv_wtasks++;
		avg_wtask_time += (1/((double) rcv_wtasks)) * (elapsed - avg_wtask_time);
		time_elapsed_svc += (end_ticks - start_ticks);
		// if type == WIN_UPDATE and finalized=true, we emit p1 to the collector
		if(w_task->type == WIN_UPDATE && w_task->finalize) {
			rcv_win_finalized++;
			avg_win_results += (1/((double) rcv_win_finalized)) * ((w_task->p1)->getSize() - avg_win_results);
			Rset_t *result_win = w_task->p1;
			ff_monode_t<w_task_t, Rset_t>::ff_send_out_to((Rset_t *) w_task, 0); // the WLQ task is the feedback message itself
			ff_monode_t<w_task_t, Rset_t>::ff_send_out_to(result_win, 1); // the results set of the window is transmitted to the collector
			return ff_monode_t<w_task_t, Rset_t>::GO_ON;
		}
		else {
			ff_monode_t<w_task_t, Rset_t>::ff_send_out_to((Rset_t *) w_task, 0); // the WLQ task is the feedback message itself
			return ff_monode_t<w_task_t, Rset_t>::GO_ON;
		}
	}

	// return the total amount of ticks spent in the svc from the beginning of the execution
	virtual ticks getsvcticks() { return time_elapsed_svc; }

	// return the total number of svc calls from the beginning of the execution
	virtual size_t getnumtask() { return rcv_wtasks; }
};

/*! 
 *  \class WLQ_Emitter
 *  
 *  \brief Emitter of the Window-level Query Stage
 *  
 *  Emitter functionality of the WLQ stage of the Pane Farming. It is responsible to
 *  receive results sets of the pane partitions from the PLQ stage and to distribute
 *  WLQ tasks to the WLQ workers.
 *  
 *  This class is defined in \ref Pane_Farming/include/wlq.hpp
 */
template<typename Rset_t>
class WLQ_Emitter: public ff_node_t<Rset_t, WLQ_Task_t<Rset_t>> {
private:
	typedef WLQ_Task_t<Rset_t> w_task_t;
	// struct of a window descriptor
	struct Win_Descr_t {
		Rset_t *w; // pointer to the results set of the window
		size_t first_pane_id; // id of the first pane of the window (starting from 0)
		size_t wp; // number of panes of the window
		int *cnt_ins; // cnt_ins[i] is the number of partitions of the i-th pane of the window (-1 if not specified yet)
		int cnt_tasks_com; // number of WLQ tasks completed of the window
		vector<Rset_t *> pendingQueue; // vector of the results sets to be computed (pending)
		bool isBusy = false; // if true a WLQ task of type (WIN_UPDATE) of this window is currently executed

		// constructor
		Win_Descr_t(size_t _id, size_t _wp, size_t _first) {
			w = new Rset_t(1, _id, 1);
			wp = _wp;
			first_pane_id = _first;
			cnt_ins = new int[wp];
			fill_n(cnt_ins, wp, -1);
			cnt_tasks_com = 0;
		}

		// destructor
		~Win_Descr_t() {
			// remember: not delete the window!
			delete cnt_ins;
		}

		// method to check if the window must be finalized
		inline bool toFinalize() {
			int sum = 0;
			for(int i=0; i<wp; i++) {
				if(cnt_ins[i] == -1) {
					sum = -1;
					break;
				}
				else sum += cnt_ins[i];
			}
			// the window must be finalized if we need to complete just one WLQ task of it
			return (sum == (cnt_tasks_com+1));
		}

		// method to update the counter of the received pane partitions of the window
		inline void updateCount(Rset_t *p) {
			// find the local id of the pane
			size_t lp_id = p->getId() - first_pane_id;
			// if the total number of partitions of pane lp_id is not known
			if(cnt_ins[lp_id] == -1 && p->getNoInstances() > 0) {
				cnt_ins[lp_id] = p->getNoInstances();
			}
		}

		// method to increment the number of completed WLQ tasks of the window
		inline void incrementCompletedTask() { cnt_tasks_com++; }

		// method to get the id of the window of this descriptor
		inline size_t getId() { return w->getId(); }
	};
	size_t wp; // number of panes in each window
	size_t sp; // number of panes in each window slide
	size_t nw_wlq; // number of activated WLQ workers
	size_t last_w = 0; // id of the last WLQ worker used by the scheduling strategy
	size_t first_win_id = 0; // id of the first window in the deque descrSet
	deque<Win_Descr_t *> descrSet; // deque of open window descriptors
	size_t n_pendingtasks = 0; // number of pending WLQ tasks to be computed for the windows
	vector<bool> workerReady; // workerReady[i] = true means that the i-th WLQ worker is ready to receive a new window task
	size_t n_readyWorkers; // number of ready workers
	ff_loadbalancer * const lb; // load balancer object for the scheduling to WLQ workers
	size_t eos_received = 0; // number of EOS messages received by the PLQ workers
	atomic<WLQ_Snapshot *> snap; // atomic pointer to the monitoring snapshot of the WLQ used for the current control step
	vector<ticks> time_ws; // vector of ticks for each WLQ worker (getsvcticks statistics)
	vector<size_t> tasks_ws; // vector of window tasks for each WLQ worker (getnumtask statistics)
	vector<bool> activatedWorker; // activatedWorker[i] = true means that the i-th WLQ worker is currently activated
	bool wlqTerminated = false; // if this flag is true the WLQ stage is terminated (or under temination)

	// method to select an activated and ready (idle) WLQ worker for a new window task, -1 if no idle worker exists
	inline int selectIdleWorker() {
		for(size_t i=last_w+1; i<NUM_WORKER_THREADS; i++) {
			if(activatedWorker[i] && workerReady[i]) {
				last_w = i;
				return i;
			}
		}
		for(size_t i=0; i<=last_w; i++) {
			if(activatedWorker[i] && workerReady[i]) {
				last_w = i;
				return i;
			}
		}
		return -1;
	}

	// method to get the number of activated and ready (idle) workers
	inline size_t getNumIdleWorkers() {
		size_t count = 0;
		for(size_t i=0; i<NUM_WORKER_THREADS; i++) {
			if(activatedWorker[i] && workerReady[i]) count++;
		}
		return count;
	}

	// method to start utilizing additional num>0 WLQ workers with given identifiers
	void addWorkers(size_t num, vector<size_t>& w_ids) {
		nw_wlq = nw_wlq + num;
		for(auto const& w: w_ids) {
			if(activatedWorker[w]) abort(); // the WLQ worker must be previously de-activated!
			activatedWorker[w] = true;
			lb->thaw(w, true);
		}
	}

	// method to stop utilizing num>0 WLQ workers with given identifiers
	void removeWorkers(size_t num, vector<size_t>& w_ids) {
		nw_wlq = nw_wlq - num;
		for(auto const& w: w_ids) {
			if(!activatedWorker[w]) abort(); // the WLQ worker must be previously activated!
			activatedWorker[w] = false;
			lb->ff_send_out_to(GO_OUT, w);
		}
	}

	// method to send a WLQ task to a selected WLQ worker
	inline void sendTaskToWorker(w_task_t *wtask, size_t worker_id) {
		// update the monitoring snapshot
		(snap.load())->tasks_to_w[worker_id]++;
		if(wtask->type == WIN_UPDATE) (snap.load())->cnt_win_tasks++;
		else if(wtask->type == MERGE) (snap.load())->cnt_merge_tasks++;
		else abort();
		workerReady[worker_id] = false; // the worker is not ready anymore
		n_readyWorkers--;
		wtask->worker_id = worker_id; // write the worker_id in the WLQ task (it was -1)
		while(!lb->ff_send_out_to(wtask, worker_id)); // send the WLQ task to the worker
		// if the window must be finalized we can destroy the descriptor
		if(wtask->finalize) {
			size_t lw_id = wtask->win_id - first_win_id;
			delete descrSet[lw_id];
			descrSet[lw_id] = nullptr; // nullptr corresponds to a finalized window
		}
	}

	// method to find new WLQ tasks to execute (priority to WIN_UPDATE tasks)
	inline vector<w_task_t *> getWTasksToSchedule(size_t n_tasks) {
		size_t n_found = 0;
		vector<w_task_t *> found_tasks;
		if(n_found == n_tasks) return found_tasks;
		// Phase 1: looking for WIN_UPDATE tasks
		for(auto &wd: descrSet) {
			if(wd != nullptr) {
				if((!wd->isBusy) && ((wd->pendingQueue).size() > 0)) {
					// create a WIN_UPDATE task
					Rset_t *p = (wd->pendingQueue).back();
	        		(wd->pendingQueue).pop_back();
					w_task_t *task = new w_task_t(WIN_UPDATE, wd->w, p, wd->getId(), wd->toFinalize());
					found_tasks.push_back(task);
					n_found++;
					if(n_found == n_tasks) return found_tasks;
				}
			}
		}
		// Phase 2: looking for MERGE tasks
		for(auto &wd: descrSet) {
			if(wd != nullptr) {
				if((wd->pendingQueue).size() > 1) {
					// create a MERGE task
					Rset_t *p1 = (wd->pendingQueue).back();
	        		(wd->pendingQueue).pop_back();
	        		Rset_t *p2 = (wd->pendingQueue).back();
	        		(wd->pendingQueue).pop_back();
					w_task_t *task = new w_task_t(MERGE, p1, p2, wd->getId(), false);
					found_tasks.push_back(task);
					n_found++;
					if(n_found == n_tasks) return found_tasks;
				}
			}
		}
		return found_tasks;
	}

	// check termination
	inline bool checkTermination() {
		/* We can propagate the EOS if we have received all the EOS from the PLQ workers,
         * there is no results set in any pending queue, and all the WLQ workers are ready. */
        if((eos_received == NUM_WORKER_THREADS) && (n_pendingtasks == 0) && (n_readyWorkers == NUM_WORKER_THREADS)) {
        	// wake up all the de-activated WLQ workers
        	for(int i=0; i<NUM_WORKER_THREADS; i++) {
        		if(!activatedWorker[i]) {
        			activatedWorker[i] = true;
        			lb->thaw(i, true); // it wakes up the i-th WLQ worker
        		}
        	}
        	return true;
        }
        else return false;
	}

	/* 
	 * Function to compute the utilization factor and the idle ratio of the WLQ stage. It is
	 * called by the elastic controller.
	 */
	pair<double, double> computeWLQMetrics(size_t INTERVAL_USECS, WLQ_Snapshot *s) {
		int max_nw_wlq = NUM_WORKER_THREADS;
		pair<double, double> result;
		// statistics per sampling interval of length INTERVAL_USECS
		double probs[max_nw_wlq]; // probs[i] is the probability to schedule a WLQ task to the i-th WLQ worker
		double mus[max_nw_wlq]; // mus[i] is the ideal number of WLQ tasks that the i-th WLQ worker can process
		double time_idle[max_nw_wlq]; // time_idle[i] is the length (usec) of the idle fraction of INTERVAL_USECS of the i-th WLQ worker
		double avg_ts = 0; // estimated average processing time per WLQ task (usec)
		double tot_time = 0; // sum of the time intervals of length INTERVAL_USECS of the activated workers only
		double tot_time_idle = 0; // sum of the idle time intervals of the activated workers only
		double tot_time_working = 0; // sum of the time intervals where all the workers are effectively processing WLQ tasks
		double tot_processed_tasks = 0; // sum of the number of WLQ tasks processed by all the workers
		double tot_tasks_scheduled = 0; // total number of scheduled tasks to all the workers
		// compute probabilities
		for(size_t i=0; i<max_nw_wlq; i++) tot_tasks_scheduled += s->tasks_to_w[i];
		for(size_t i=0; i<max_nw_wlq; i++) probs[i] = s->tasks_to_w[i] / tot_tasks_scheduled;
		// get the pointers to the WLQ workers
		const svector<ff_node*> &workers = lb->getWorkers();
		for(size_t i=0; i<max_nw_wlq; i++) {
			WLQ_Worker<Rset_t> *worker_casted = static_cast<WLQ_Worker<Rset_t> *>(workers[i]);
			// total time (in ticks) passed in the svc by the i-th WLQ worker from the beginning of the execution
			ticks active_time_ticks = worker_casted->getsvcticks();
			// time passed in processing WLQ tasks by the i-th WLQ worker during the last INTERVAL_USECS
			double time_working = FROM_TICKS_TO_USECS(active_time_ticks - time_ws[i]);
			tot_time_working += time_working;
			// idle time of the i-th WLQ worker
			time_idle[i] = std::max(0.0, INTERVAL_USECS - time_working);
			// total number of WLQ tasks processed by the i-th WLQ worker from the beginning of the execution
			int tasks = worker_casted->getnumtask();
			// number of WLQ tasks processed by the i-th WLQ worker during the last INTERVAL_USECS
			mus[i] = tasks - tasks_ws[i];
			tot_processed_tasks += mus[i];
			// save the last statistics
			tasks_ws[i] = tasks;
			time_ws[i] = active_time_ticks;
		}
		// compute the estimated average processing time per WLQ task
		avg_ts = tot_time_working / tot_processed_tasks;
		// compute the utilization factor of the WLQ
		double rho = 0;
		for(size_t i=0; i<max_nw_wlq; i++) {
			// we take into account only the activated workers
			if(activatedWorker[i]) {
				mus[i] += time_idle[i] / avg_ts;
				rho += probs[i] * (((tot_tasks_scheduled + n_pendingtasks) * probs[i]) / mus[i]);
			}
		}
		if(std::isnan(rho) || std::isinf(rho)) result.first = 0;
		else result.first = rho;
		// compute the idle ratio of the WLQ workers
		for(size_t i=0; i<max_nw_wlq; i++) {
			// we take into account only the activated workers
			if(activatedWorker[i]) {
				tot_time_idle += time_idle[i];
				tot_time += INTERVAL_USECS;
			}
		}
		result.second = tot_time_idle / tot_time;
		return result;
	}

public:
	// constructor
	WLQ_Emitter(size_t _wp, size_t _sp, size_t _nw_wlq, ff_loadbalancer * const _lb)
	            :wp(_wp), sp(_sp), nw_wlq(_nw_wlq), n_readyWorkers(NUM_WORKER_THREADS), lb(_lb), workerReady(NUM_WORKER_THREADS, true), snap(new WLQ_Snapshot()), time_ws(NUM_WORKER_THREADS, 0),
	             tasks_ws(NUM_WORKER_THREADS, 0), activatedWorker(NUM_WORKER_THREADS, true) {
		fill_n(activatedWorker.begin(), NUM_WORKER_THREADS-nw_wlq, false); // the first NUM_WORKER_THREADS-nw_wlq workers are initially not utilized
	}

	// destructor
	~WLQ_Emitter() {
		delete snap.load();
	}

	// svc_init method
	int svc_init() {
		// set the thread mapping onto a SMT context
		ff_mapThreadToCpu(WLQ_EMITTER_CORE_ID);
		// de-activate all the initially non-utilized WLQ workers
		for(int i=0; i<NUM_WORKER_THREADS-nw_wlq; i++) {
			lb->ff_send_out_to(GO_OUT, i);
		}
		// entering the sampling barrier
		pthread_barrier_wait(&sampleBarr);
		return 0;
	}

	// svc_end method
	void svc_end() {
		// entering the printing barrier
		pthread_barrier_wait(&printBarr);
		// entering the precedence barrier 1
		pthread_barrier_wait(&precBarr1);
		// check whether there are windows to be finalized within the WLQ emitter
		for(auto const& wd: descrSet) {
			if(wd != nullptr) {
				if(wd->toFinalize() && (wd->pendingQueue).size() > 0) MY_PRINT("WLQ emitter has some windows to be finalized!\n");
				break;
			}
		}
		// entering the precedence barrier 2
		pthread_barrier_wait(&precBarr2);
		// entering the precedence barrier 3
		pthread_barrier_wait(&precBarr3);
	}

	// method to properly manage the EOS
	void eosnotify(ssize_t id) {
		// if the EOS comes from a PLQ worker
		if(id == -1) {
			// we increase the counter and eventually propagate the EOS to the WLQ workers
            eos_received++;
            if(checkTermination()) { // <-- this call eventually wakes up all the de-activated WLQ workers
            	wlqTerminated = true;
            	lb->broadcast_task(ff_node_t<Rset_t, w_task_t>::EOS);
            }
        }
    }

	// svc method
	w_task_t *svc(Rset_t *p) {
		// identify the source of the input message
		int msg_source = lb->get_channel_id();
        // the input message is a new results set of a pane partition from the PLQ stage
        if(msg_source == -1) {
			/* 
			 * Actions to be executed:
			 * 1-insert the results set p into the pending queues of all the windows that contain it;
			 * 2-determine the number n>=0 of idle workers;
			 * 3-look for n WLQ tasks (priority to WIN_UPDATE tasks);
			 * 4-schedule the found tasks to the idle workers.
			 */
        	// get the id of the pane of the results set (starting from 0)
			size_t pane_id = p->getId();
			// determine the id of the first and last window that the results set p belongs to
			size_t first_win;
			if(pane_id < wp) first_win = 0;
			else first_win = ceil(((double) ((pane_id + 1) - wp)) / sp);
			size_t last_win = ceil(((double) (pane_id + 1)) / sp) - 1;
			// new window descriptors may be created
			if(last_win - first_win_id >= descrSet.size()) {
				size_t old_size = descrSet.size();
				size_t new_size = last_win - first_win_id + 1;
				descrSet.resize(new_size);
				for(int i=old_size; i<new_size; i++) {
					// compute the id of the window to insert in the new descriptor
					size_t w_id = i + first_win_id;
					// compute the id of the first pane of the window
					size_t first_pane_id = sp * w_id;
					descrSet[i] = new Win_Descr_t(w_id, wp, first_pane_id);
				}
			}
 			// add the results set p to the pending queues of all the windows to which it belongs to
			for(size_t i=first_win - first_win_id; i<=last_win - first_win_id; i++) {
				if(descrSet[i] == nullptr) abort(); // if the descriptor of the window does not exist abort!
				// update the number of pane partitions of the window
				descrSet[i]->updateCount(p);
				// insert the results set p into the pending queue of the window
				(descrSet[i]->pendingQueue).push_back(p);
				n_pendingtasks++;
			}
			// determine the number of idle workers
			size_t n_idle_w = getNumIdleWorkers();
			// find at most n_idle_w WLQ tasks to be scheduled
			vector<w_task_t *> tasks_vect = getWTasksToSchedule(n_idle_w);
			for(auto wtask: tasks_vect) {
				// get the identifier of an idle worker (it must exist)
				size_t worker_id = selectIdleWorker();
				if(worker_id == -1) abort();
				if(wtask->type == WIN_UPDATE) {
					size_t lw_id = wtask->win_id - first_win_id;
					descrSet[lw_id]->isBusy = true;
					n_pendingtasks--;
				}
				else n_pendingtasks--;
				sendTaskToWorker(wtask, worker_id);
			}
			// delete the window descriptors of finalized windows
			auto it = descrSet.begin();
			for(; it<descrSet.end(); it++) {
				if((*it) == nullptr) first_win_id++;
				else break;
			}
			descrSet.erase(descrSet.begin(), it);
			return ff_node_t<Rset_t, w_task_t>::GO_ON;
		}
        // the input message is a feedback from one of the WLQ workers
        else if(msg_source >= 0 && msg_source < lb->getNWorkers()) {
			/* 
			 * Actions to be executed:
			 * 1-in case of a MERGE task, we add the results set p1 to the pending pane of the window;
			 * 2-in case of a WIN_UPDATE task, we mark the window as not busy (if it was not finalized);
			 * 3-if the worker is still activated, we look for one WLQ task to execute (priority to WIN_UPDATE tasks);
			 * 4-schedule the found task to the worker (if it is still activated), otherwise the worker becomes ready.
			 */
        	w_task_t *fb = reinterpret_cast<w_task_t *>(p); // ugly but needed!
        	// get the local index in deque of the window of the task
	        size_t lw_id = fb->win_id - first_win_id;
	        // if it was a WIN_UPDATE, the window (if not finalized) is not busy anymore
        	if(!fb->finalize && fb->type == WIN_UPDATE) {
        		descrSet[lw_id]->isBusy = false;
        		descrSet[lw_id]->incrementCompletedTask();
        	}
        	else if(fb->type == MERGE) {
    			// if it was a MERGE, p1 is added to the pending queue
        		(descrSet[lw_id]->pendingQueue).push_back(fb->p1);
        		descrSet[lw_id]->incrementCompletedTask();
        	}
 			// the worker becomes ready (and idle)
			workerReady[fb->worker_id] = true;
			n_readyWorkers++;
        	// if the worker is still activated
        	if(activatedWorker[fb->worker_id]) {
	        	// find one WLQ task to be scheduled
				vector<w_task_t *> tasks_vect = getWTasksToSchedule(1);
				// if we have one WLQ task to assign to the WLQ worker
				if(tasks_vect.size() == 1) {
					w_task_t *wtask = tasks_vect[0];
					if(wtask->type == WIN_UPDATE) {
						size_t lw_id = wtask->win_id - first_win_id;
						descrSet[lw_id]->isBusy = true;
						n_pendingtasks--;
					}
					else n_pendingtasks--;
					sendTaskToWorker(wtask, fb->worker_id);
				}
        	}
        	delete fb;
        	// handling termination
        	if(checkTermination()) { // <-- this call eventually wakes up all the de-activated WLQ workers
 				wlqTerminated = true;
        		lb->broadcast_task(ff_node_t<Rset_t, w_task_t>::EOS);
        	}
        	else return ff_node_t<Rset_t, w_task_t>::GO_ON;
		}
        // the input message is a reconfiguration command from the elastic controller
        else {
        	Reconf_t *reconf_msg = reinterpret_cast<Reconf_t*>(p); // ugly but needed!
			// we have a reconfiguration message
			switch(reconf_msg->type) {
				case ADD: {
					addWorkers((reconf_msg->w_ids).size(), (reconf_msg->w_ids));
					break;
				}
				case REMOVE: {
					removeWorkers((reconf_msg->w_ids).size(), (reconf_msg->w_ids));
					break;
				}
				default: {
					cout << "Error: WLQ emitter does not recognize the reconfiguration command!" << endl;
					abort();
				}
			}
			delete reconf_msg;
			return ff_node_t<Rset_t, w_task_t>::GO_ON;
		}
	}

	// method to know if the WLQ stage is terminated (or under termination)
	bool isTerminated() { return wlqTerminated; }

	// method to get the number of pending tasks internally stored by the WLQ emitter
	size_t getPendingTasks() { return n_pendingtasks; }

	// method to acquire the monitored data of the WLQ stage (called by the elastic controller)
	WLQ_Monitored_Data getMonitoredData() {
		// create a new monitoring snapshot that will be atomically swapped with the old one
		WLQ_Snapshot *old_snap = snap.exchange(new WLQ_Snapshot());
		pair<double, double> result = computeWLQMetrics(CONTROL_STEP_USEC, old_snap);
		size_t win_tasks = old_snap->cnt_win_tasks;
		size_t merge_tasks = old_snap->cnt_merge_tasks;
		delete old_snap;
		return WLQ_Monitored_Data(result.first, result.second, win_tasks, merge_tasks);
	}
};

/*! 
 *  \class WLQ_Collector
 *  
 *  \brief Collector of the Window-level Query Stage
 *  
 *  Collector functionality of the WLQ stage of the Pane Farming. It is
 *  in charge of receiving the results sets of windows from the WLQ worker
 *  and producing them outside in the increasing order of their identifier.
 *  
 *  This class is defined in \ref Pane_Farming/include/wlq.hpp
 */
template<typename Rset_t>
class WLQ_Collector: public ff_node_t<Rset_t> {
private:
	// identifier of the next results set of a window to transmit
	size_t next_win_id = 0; // window id starting from 0
	// deque of the results sets of windows received and buffered for ordering
	deque<Rset_t *> winSet;
	int count = 0;

public:
	// constructor
	WLQ_Collector() {}

	// destructor
	~WLQ_Collector() {}

	// svc_init method
	int svc_init() {
		// set the thread mapping onto a SMT context
		ff_mapThreadToCpu(WLQ_COLLECTOR_CORE_ID);
		return 0;
	}

	// svc_end method
	void svc_end() {
		// check whether there are some pending results sets of some windows in deque
		if(winSet.size() != 0) {
			MY_PRINT("WLQ collector has some pending results sets of some windows in deque!\n");
		}
	}

	// svc method
	Rset_t *svc(Rset_t *w) {
#if !defined(UNORDERING_COLLECTOR)
		// if the received results set of a window is the next one to transmit
		if(w->getId() == next_win_id) {
			next_win_id++;
			ff_node_t<Rset_t>::ff_send_out(w);
			if(winSet.size() > 0) {
				// check whether there are other results sets of some windows to transmit
				auto it = winSet.begin() + 1;
				for(; it<winSet.end(); it++) {
					if((*it) == nullptr) break;
					else {
						next_win_id++;
						ff_node_t<Rset_t>::ff_send_out(*it);
					}
				}
				// erase the pointers to the transmitted results set in deque
				winSet.erase(winSet.begin(), it);
			}
		}
		// otherwise the results set must be buffered
		else {
			if(w->getId() - next_win_id >= winSet.size()) {
				size_t old_size = winSet.size();
				size_t new_size = w->getId() - next_win_id + 1;
				winSet.resize(new_size, nullptr);
			}
			winSet[w->getId() - next_win_id] = w;
		}
		return ff_node_t<Rset_t>::GO_ON;
#else
		// simply forward the results set of the window to the next stage in the arrival order
		return w;
#endif
	}
};

#endif
