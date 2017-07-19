/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file controller.hpp
 *  \brief Controller of the Elastic Pane Farming Pattern
 *  
 *  Implementation of the controller entity which periodically acquires
 *  monitored data from the PLQ and the WLQ stages and sends reconfiguration
 *  commands in order to increase/decrease their parallelism degree according
 *  to an adaptation strategy implemented internally by the controller.
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
 * Dec 2016
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

// include
#include <stdlib.h>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <ff/node.hpp>
#include <ff/utils.hpp>
#include <plq.hpp>
#include <wlq.hpp>
#include <fuzzy.hpp>
#include <general.hpp>

using namespace ff;
using namespace std;

/* 
 * This file provides the following classes:
 # -ResourcePool: class implementing the resource pool used by the controller;
 * -Controller: class implementing the Pane Farming controller.
 */

/* 
 * Enumeration of the possible owners of a resource:
 * -PLQ = resource owned by the PLQ stage;
 * -WLQ = resource owned by the WLQ stage;
 * -NONE = resource available.
 */
enum owner_t { PLQ, WLQ, NONE };

/*! 
 *  \class ResourcePool
 *  
 *  \brief Resource Pool used by the Controller
 *  
 *  Class implementing the functionalities for dynamically managing the SMT contexts (resources)
 *  available to execute the workers of the PLQ and WLQ stages.
 *  
 *  This class is defined in \ref Pane_Farming/include/controller.hpp
 */
class ResourcePool {
private:
	// struct of a resource (it models a SMT context)
	struct Resource_t {
		size_t context_id; // identifier of the SMT context of the resource
		owner_t owner; // owner of the resource

		// constructor
		Resource_t(size_t _context_id, owner_t _owner=NONE): context_id(_context_id), owner(_owner) {}
	};
	// pool of resources
	vector<Resource_t> pool;
	ff_loadbalancer * const plq_lb; // pointer to the PLQ load balancer
	ff_loadbalancer * const wlq_lb; // pointer to the WLQ load balancer

public:
	// constructor
	ResourcePool(ff_loadbalancer * const _plq_lb, size_t nw_plq, ff_loadbalancer * const _wlq_lb, size_t nw_wlq): plq_lb(_plq_lb), wlq_lb(_wlq_lb) {
		// create the pool of resources with the correct SMT context identifiers
		size_t i = 0;
		size_t count = 0;
#if defined(PLQ_ONLY)
        size_t start_idx = CLIENT_CORE_ID + CORE_OFFSET;
#else
        size_t start_idx = WLQ_COLLECTOR_CORE_ID + CORE_OFFSET;
#endif
        int totW = NUM_WORKER_THREADS;
        while(i <= (NUM_WORKER_THREADS/NUM_WORKER_CORES)) {
            for(size_t j=0; j<min(totW, NUM_WORKER_CORES); j++) {
            	// the resource is already used by the PLQ
            	if(count < nw_plq) {
            		Resource_t res(start_idx + (i*CONTEXT_OFFSET) + (j*CORE_OFFSET), PLQ);
            		pool.push_back(res);
            	}
#if !defined(PLQ_ONLY)
            	// the resource is already used by the WLQ
            	else if(count >= NUM_WORKER_CORES-nw_wlq) {
            		Resource_t res(start_idx + (i*CONTEXT_OFFSET) + (j*CORE_OFFSET), WLQ);
            		pool.push_back(res);
            	}
#endif
            	// the resource is available
            	else {
            		Resource_t res(start_idx + (i*CONTEXT_OFFSET) + (j*CORE_OFFSET), NONE);
            		pool.push_back(res);
            	}
            	count++;
            }
            totW = totW - min(totW, NUM_WORKER_CORES);
            i++;
        }
	}

	// destructor
	~ResourcePool() {}

	/* 
	 * Method to acquire num>0 available resources from the pool
	 * by a specific owner. It may return a smaller number of available
	 * resources according to the current resource availability.
	 */
	vector<size_t> acquireResources(size_t num, owner_t new_owner) {
		vector<size_t> ids; // identifiers of the acquired resources in the pool
		size_t acquired = 0;
		size_t id = 0;
		for(Resource_t &r: pool) {
			if((r.owner == NONE) && (acquired < num)) {
				r.owner = new_owner;
				acquired++;
				ids.push_back(id);
			}
			id++;
		}
		return ids;
	}

	/* 
	 * Method to release num>0 resources with a certain owner (they must exist).
	 * The implementation releases the resources with that owner that are currently
	 * less utilized.
	 */
	vector<size_t> releaseResources(size_t num, owner_t old_owner) {
		vector<size_t> ids; // identifiers of the released resources in the pool
		size_t released = 0;
		const svector<ff_node*> &workers = (old_owner == PLQ) ? plq_lb->getWorkers() : wlq_lb->getWorkers();
		while(released < num) {
			size_t id = 0;
			size_t min = numeric_limits<size_t>::max();
			size_t res_id = 0;
			Resource_t *res;
			bool found = false;
			// find the least loaded resource currently owned by old_owner
			for(Resource_t &r: pool) {
				if(r.owner == old_owner) {
					FFBUFFER *q = workers[id]->get_in_buffer();
					size_t qlen = q->length();
					if(min > qlen) {
						min = qlen;
						res = &r;
						res_id = id;
						found = true;
					}
				}
				id++;
			}
			if(!found) abort();
			// release the found resource
			res->owner = NONE;
			released++;
			ids.push_back(res_id);
		}
		if(released != num) abort();
		return ids;
	}
};

/*! 
 *  \class Controller
 *  
 *  \brief Controller of the Elastic Pane Farming Pattern
 *  
 *  Implementation of the controller entity which periodically acquires
 *  monitored data from the PLQ and the WLQ stages and sends reconfiguration
 *  commands in order to increase/decrease their parallelism degree
 *  according to an adaptation strategy.
 *  
 *  This class is defined in \ref Pane_Farming/include/controller.hpp
 */
template <typename PLQ_Emitter_T, typename WLQ_Emitter_T>
class Controller: public ff_node_t<Reconf_t> {
private:
	ff_loadbalancer * const plq_lb; // pointer to the PLQ load balancer
	PLQ_Emitter_T *plq_emitter; // pointer to the PLQ emitter
	ff_buffernode *ch_plq; // pointer to a fastflow channel connected to the PLQ emitter
	ff_loadbalancer * const wlq_lb; // pointer to the WLQ load balancer
	WLQ_Emitter_T *wlq_emitter; // pointer to the WLQ emitter
	ff_buffernode *ch_wlq; // pointer to a fastflow channel connected to the WLQ emitter
	size_t nw_plq, nw_wlq; // parallelism degrees of the two stages
	ResourcePool rPool; // resource pool
	size_t n_reconf = 0; // number of control steps with a reconfiguration
	size_t n_control_steps = 0; // number of control steps
	double avg_plq_rho = 0; // average rho of the PLQ over the entire execution
	double avg_wlq_rho = 0; // average rho of the WLQ over the entire execution
	double avg_worker_plq = 0; // average no. of PLQ workers utilized
	double avg_worker_wlq = 0; // average no. of WLQ workers utilized
	volatile ticks start_ticks; // ticks of the starting time of the controller
	FuzzyStrategy fuzzy_strategy; // fuzzy adaptation strategy used by the controller
#if defined(LOG)
	// create the statistics log file of the controller
	ofstream controller_logfile;
	ostringstream controller_logstream;
#if defined(ENERGY)
	Joules past_joules = 0; // number of joules consumed at the end of the previous control step
#endif
#endif

	/* 
	 * Method to translate the result of the adaptation stategy evaluation into
	 * proper reconfiguration messages to be eventually transmitted to the two stages.
	 */
	pair<Reconf_t *, Reconf_t *> get_reconfs_from_strategy(double plq_rho, double plq_sigma, double wlq_rho) {
		pair<Reconf_t *, Reconf_t *> reconfs;
		pair<size_t, size_t> new_nws; // new parallelism degrees of the PLQ and WLQ stages
#if defined(PLQ_ONLY)
		// neutral value for the adaptation strategy
		wlq_rho = 0.9;
		// neutral value for the adaptation strategy
		plq_sigma = 1.0;
#endif
#if defined(TB_RR_SCHED) or defined(ADAPTIVE_SCHED)
		// neutral value for the adaptation strategy
		plq_sigma = 1.0;
#endif
		// evaluation of the fuzzy adaptation strategy
		new_nws = fuzzy_strategy.evaluate(plq_rho, plq_sigma, wlq_rho, nw_plq, nw_wlq);
#if defined(PLQ_ONLY)
		// the second stage does not exist if PLQ_ONLY is enabled
		new_nws.second = 0;
#endif
		if(plq_emitter->isTerminated()) new_nws.first = 0; // the first stage is terminated, we remove all the activated PLQ workers
		// PLQ stage
		if(new_nws.first == nw_plq) reconfs.first = new Reconf_t(); // no reconfiguration
		else if(new_nws.first < nw_plq) {
			// we need to remove some PLQ workers
			int deltaW = nw_plq - new_nws.first;
			reconfs.first = new Reconf_t(REMOVE, rPool.releaseResources(deltaW, PLQ));
		}
		else {
			// we need to add some PLQ workers
			int deltaW = new_nws.first - nw_plq;
			vector<size_t> w_ids = rPool.acquireResources(deltaW, PLQ);
			if(w_ids.size() == 0) reconfs.first = new Reconf_t();
			else reconfs.first = new Reconf_t(ADD, w_ids);
		}
		// WLQ stage
		if(new_nws.second == nw_wlq) reconfs.second = new Reconf_t(); // no reconfiguration
		else if(new_nws.second < nw_wlq) {
			// we need to remove some WLQ workers
			int deltaW = nw_wlq - new_nws.second;
			reconfs.second = new Reconf_t(REMOVE, rPool.releaseResources(deltaW, WLQ));
		}
		else {
			// we need to add some WLQ workers
			int deltaW = new_nws.second - nw_wlq;
			vector<size_t> w_ids = rPool.acquireResources(deltaW, WLQ);
			if(w_ids.size() == 0) reconfs.second = new Reconf_t();
			else reconfs.second = new Reconf_t(ADD, w_ids);
		}
		return reconfs;
	}

public:
	// constructor
	Controller(ff_loadbalancer * const _plq_lb, PLQ_Emitter_T *_plq_emitter, ff_buffernode *_ch_plq, ff_loadbalancer * const _wlq_lb, WLQ_Emitter_T *_wlq_emitter, ff_buffernode *_ch_wlq, size_t _nw_plq, size_t _nw_wlq)
	           :plq_lb(_plq_lb), plq_emitter(_plq_emitter), ch_plq(_ch_plq), wlq_lb(_wlq_lb), wlq_emitter(_wlq_emitter), ch_wlq(_ch_wlq), nw_plq(_nw_plq), nw_wlq(_nw_wlq), rPool(plq_lb, nw_plq, wlq_lb, nw_wlq) {}

	// destructor
	~Controller() {}

	// svc_init method
	int svc_init() {
		// check the size of some macros
		if((SAMPLING_ASPID_PLQ_USEC > CONTROL_STEP_USEC) || (CONTROL_STEP_USEC % SAMPLING_ASPID_PLQ_USEC !=0)) {
			cout << "Error: choose a correct value for the macros CONTROL_STEP_USEC and SAMPLING_ASPID_PLQ_USEC!" << endl;
			exit(1);
		}
#if defined(LOG)
		// open the statistics log file of the controller
		controller_logfile.open(CONTROLLER_LOG_FILENAME);
		controller_logstream << "#control_step plq_rho plq_splitting wlq_rho nw_plq nw_wlq wlq_idle wlq_win_tasks wlq_merge_tasks [watts]\n";
#endif
		// select the thread mapping onto a CPU thread context
		ff_mapThreadToCpu(CONTROLLER_CORE_ID);
		// entering the sampling barrier
		pthread_barrier_wait(&sampleBarr);
		start_ticks = getticks();
		return 0;
	}

	// svc_end method
	void svc_end() {
		// entering the printing barrier
		pthread_barrier_wait(&printBarr);
		// entering the precedence barrier 1
		pthread_barrier_wait(&precBarr1);
#if !defined(PLQ_ONLY)
		// entering the precedence barrier 2
		pthread_barrier_wait(&precBarr2);
#endif
		MY_PRINT("*****************************Controller statistics****************************\n" <<\
		         "No. of reconfigurations: " << n_reconf << "\n" <<\
		         "Average utilization factor (PLQ): " << avg_plq_rho << "\n" <<\
		         "Average no. of PLQ workers: "  << avg_worker_plq << "\n" <<\
		         "Average utilization factor (WLQ): " << avg_wlq_rho << "\n" <<\
		         "Average no. of WLQ workers: "  << avg_worker_wlq << "\n" <<\
		         "******************************************************************************\n");
#if defined(PLQ_ONLY)
		// entering the precedence barrier 2
		pthread_barrier_wait(&precBarr2);
#else
		// entering the precedence barrier 3
		pthread_barrier_wait(&precBarr3);
#endif
#if defined(LOG)
		// close the statistics log file of the controller
		controller_logfile << controller_logstream.str();
 		controller_logfile.close();
#endif
	}

	// svc method
	Reconf_t *svc(Reconf_t *r) {
		size_t n_usteps = 0; // number of micro-step in a control step
		size_t waitedusec = 0; // usecs waited during a control step
		double plq_rho = 0, wlq_rho = 0;
		double plq_sigma = 0;
		double wlq_idle = 0;
		size_t cnt_win_tasks = 0, cnt_merge_tasks = 0;
		vector<size_t> completed_panes_split; // vector of splitting factors of the panes completed during a control step
		// while loop until the WLQ stage is terminated (actually it is the PLQ stage if the PLQ_ONLY macro is enabled)
		while(!wlq_emitter->isTerminated()) {
			// waiting the length of a micro-step (the one of the PID in the PLQ if present)
			usleep(SAMPLING_ASPID_PLQ_USEC);
			n_usteps++;
			// get the monitored data from the PLQ at every micro-step
			PLQ_Monitored_Data plq_data = plq_emitter->getMonitoredData();
			plq_rho += (1/((double) n_usteps)) * (plq_data.rho - plq_rho);
			completed_panes_split.reserve(completed_panes_split.size() + plq_data.split_vect.size());
			completed_panes_split.insert(completed_panes_split.end(), plq_data.split_vect.begin(), plq_data.split_vect.end());
			waitedusec += SAMPLING_ASPID_PLQ_USEC;
			if(waitedusec >= CONTROL_STEP_USEC) { // beginning of a new control step
				n_control_steps++;
#if !defined(PLQ_ONLY)
				// get monitored data from the WLQ at every control step
				WLQ_Monitored_Data wlq_data = wlq_emitter->getMonitoredData();
				wlq_rho = wlq_data.rho;
				wlq_idle = wlq_data.idle;
				cnt_win_tasks = wlq_data.cnt_win_tasks;
				cnt_merge_tasks = wlq_data.cnt_merge_tasks;
#endif
				// compute the average splitting factor during the control step:
				for(const size_t& s: completed_panes_split) plq_sigma += s;
				plq_sigma = plq_sigma / completed_panes_split.size();
				// update the average utilization factors over the entire execution (if the PLQ is still working)
				if(!plq_emitter->isTerminated()) avg_plq_rho += (1/((double) n_control_steps)) * (plq_rho - avg_plq_rho);
				avg_wlq_rho += (1/((double) n_control_steps)) * (wlq_rho - avg_wlq_rho);
#if defined(LOG)
				// update the average no. of PLQ workers utilized (if the PLQ is still working)
				if(!plq_emitter->isTerminated()) avg_worker_plq += (1/((double) n_control_steps)) * (nw_plq - avg_worker_plq);
				// update the average no. of WLQ workers utilized
				avg_worker_wlq += (1/((double) n_control_steps)) * (nw_wlq - avg_worker_wlq);
#if !defined(ENERGY)
				// update the log statistics of the controller
				controller_logstream << n_control_steps-1 << " " << plq_rho << " " << plq_sigma << " " << wlq_rho << " " << nw_plq << " " << nw_wlq << " " <<  wlq_idle << " " <<  cnt_win_tasks << " " << cnt_merge_tasks << "\n";
#else
				Joules j = energy_counter->getJoules(); // energy_counter is a global variable
				// update the log statistics of the controller with the power consumption
				controller_logstream << n_control_steps-1 << " " << plq_rho << " " << plq_sigma << " " << wlq_rho << " " << nw_plq << " " << nw_wlq << " " <<  wlq_idle << " " <<  cnt_win_tasks << " " << cnt_merge_tasks << " " << ((j - past_joules) / (CONTROL_STEP_USEC/1000000)) << "\n";
				past_joules = j;
#endif
#endif
				// obtain the reconfigurations to execute from the adaptation strategy
#if !defined NO_ELASTIC
				pair<Reconf_t *, Reconf_t *> reconfs = get_reconfs_from_strategy(plq_rho, plq_sigma, wlq_rho);
#else
				pair<Reconf_t *, Reconf_t *> reconfs;
				reconfs.first = new Reconf_t();
				reconfs.second = new Reconf_t();
#endif
				// increase the counter of reconfigurations performed
				if(((reconfs.first)->type != NORECONF) || ((reconfs.second)->type != NORECONF)) n_reconf++;
				string plq_string = "", wlq_string = "";
				// handling reconfiguration of the PLQ stage
				Reconf_t *plq_reconf = reconfs.first;
				plq_string = "-- PLQ Workers [";
				if(plq_reconf->type == ADD) {
					// update the parallelism degree of the PLQ
					size_t old_nw = nw_plq;
					nw_plq += (plq_reconf->w_ids).size();
					plq_string += to_string(old_nw) + "->" + to_string(nw_plq) + "] ADDED " + to_string((plq_reconf->w_ids).size()) + " workers";
					ch_plq->ff_send_out(plq_reconf); // send the reconfiguration message to the PLQ emitter
				}
				else if(plq_reconf->type == REMOVE) {
					// update the parallelism degree of the PLQ
					size_t old_nw = nw_plq;
					nw_plq -= (plq_reconf->w_ids).size();
					plq_string += to_string(old_nw) + "->" + to_string(nw_plq) + "] REMOVED " + to_string((plq_reconf->w_ids).size()) + " workers";
					ch_plq->ff_send_out(plq_reconf); // send the reconfiguration message to the PLQ emitter
				}
				else {
					plq_string += to_string(nw_plq) + "]";
					delete plq_reconf;
				}
#if !defined(PLQ_ONLY)
				// handling reconfiguration of the WLQ stage
				Reconf_t *wlq_reconf = reconfs.second;
				wlq_string = "-- WLQ Workers [";
				if(wlq_reconf->type == ADD) {
					// update the parallelism degree of the WLQ
					size_t old_nw = nw_wlq;
					nw_wlq += (wlq_reconf->w_ids).size();
					wlq_string += to_string(old_nw) + "->" + to_string(nw_wlq) + "] ADDED " + to_string((wlq_reconf->w_ids).size()) + " workers ";
					ch_wlq->ff_send_out(wlq_reconf); // send the reconfiguration message to the WLQ emitter
				}
				else if(wlq_reconf->type == REMOVE) {
					// update the parallelism degree of the WLQ
					size_t old_nw = nw_wlq;
					nw_wlq -= (wlq_reconf->w_ids).size();
					wlq_string += to_string(old_nw) + "->" + to_string(nw_wlq) + "] REMOVED " + to_string((wlq_reconf->w_ids).size()) + " workers ";
					ch_wlq->ff_send_out(wlq_reconf); // send the reconfiguration message to the WLQ emitter
				}
				else {
					wlq_string += to_string(nw_wlq) + "] ";
					delete wlq_reconf;
				}
#endif
				double time = ((double)(getticks()-start_ticks)/((long long)((unsigned long)(FREQ)*1000000))); // in seconds
#if(PLQ_ONLY)
				cout << fixed << setprecision(2) << "   |-->[CONTROLLER] Time: " << setw(7) << time << ", control step [" << setw(3) << n_control_steps-1 << "], PLQ [rho: " <<  ((plq_rho<1)? BOLDGREEN : RED) << setw(6) << plq_rho << RESET << ", split: " << BOLDYELLOW << plq_sigma << RESET << "], WLQ [rho: " <<  ((wlq_rho<1)? BOLDGREEN : RED) << setw(6) << wlq_rho << RESET << ", p_tasks: 0, idle_ratio: 0] " << plq_string << " " << wlq_string << endl;
#else
				cout << fixed << setprecision(2) << "   |-->[CONTROLLER] Time: " << setw(7) << time << ", control step [" << setw(3) << n_control_steps-1 << "], PLQ [rho: " <<  ((plq_rho<1)? BOLDGREEN : RED) << setw(6) << plq_rho << RESET << ", split: " << BOLDYELLOW << plq_sigma << RESET << "], WLQ [rho: " <<  ((wlq_rho<1)? BOLDGREEN : RED) << setw(6) << wlq_rho << RESET << ", p_tasks: " << setw(4) << wlq_emitter->getPendingTasks() << ", idle_ratio: " << setw(4) << wlq_idle << "] " << plq_string << " " << wlq_string << endl;
#endif
				// reset counters
				waitedusec = 0;
				n_usteps = 0;
				plq_rho = 0;
				plq_sigma = 0;
				completed_panes_split.clear();
			}
		}
		return ff_node_t<Reconf_t>::EOS;
	}

	// run method
	int run() { return ff_node_t<Reconf_t>::run(); }

	// wait method
	int wait() { return ff_node_t<Reconf_t>::wait(); }
};

#endif
