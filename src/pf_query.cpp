/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file pf_query.cpp
 *  \brief Application of the Pane Farming Pattern
 *  
 *  The application consists in a pipeline of two stages where the first one
 *  is the Pane Farming pattern while the second is a client node that
 *  collects the results received from the Pane Farming. The Pane Farming
 *  can execute several types of continuous queries depending whether some
 *  macros are enabled or not. To date, two queries are supported:
 *  1- Skyline Query: macro SKYLINE;
 *  2- Top-δ Dominant Skyline Query: macro TOPD.
 *  
 *  \note The application receives stream elements from a Generator through
 *  a TCP/IP socket.
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

// include
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <limits>
#include <iostream>
#include <ff/node.hpp>
#include <ff/mapper.hpp>
#include <ff/pipeline.hpp>
#include <plq.hpp>
#include <general.hpp>
#include <tuple_t.hpp>
#include <queries.hpp>
#include <socket_utils.hpp>
#include <pane_farming.hpp>

using namespace std;
using namespace ff;

// set the types of the Pane and Window according to the query executed
#if defined(SKYLINE)
typedef SkySet Rset_t;
#elif defined(TOPD)
typedef TopdSet Rset_t;
#endif

/*! 
 *  \class Client_PLQ (PLQ_ONLY)
 *  
 *  \brief Client class of the PLQ stage of Pane Farming
 *  
 *  Class that implements the generic client that receives the results sets of
 *  the pane partitions from the PLQ stage of the Pane Farming pattern.
 *  
 *  This class is defined in \ref Pane_Farming/include/pf_query.cpp
 */
class Client_PLQ: public ff_minode_t<Rset_t> {
private:
	volatile ticks last_print, start_ticks;
	unsigned long rcv_panes = 0; // number of completed panes over the entire execution
	unsigned long rcv_instances = 0; // number of completed pane partitions over the entire execution
	vector<int> cnt_ins; // cnt_ins[i] is the total number of partitions of the i-th pane (-1 unspecified)
	vector<int> cnt_ins_rcv; // cnt_ins_rcv[i] is the current number of partitions received of the i-th pane
	vector<int> sizes_pane_vect; // sizes_pane_vect[i] total number of tuples in the results sets of the i-th pane
	vector<double> starting_pane_vect; // starting_pane_vect[i] is the starting time in usec of the i-th pane
	vector<double> ending_pane_vect; // ending_pane_vect[i] is the ending time in usec of the i-th pane
	vector<double> closing_pane_vect; // closing_pane_vect[i] is the closing time in usec of the i-th pane
	double pane_rate = 0; // number of panes completed per second over the entire execution
	double avg_ins_per_pane = 0; // average number of partitions per pane over the entire execution
	double tot_avg_pane_size = 0; // average number of tuples selected per pane
	double tot_Sn_pane_size = 0; // n*\sigma^{2}_{n} of the number of tuples selected per pane
	double tot_avg_closing_delay = 0; // average closing delay (usec) of panes over the entire execution
	double tot_avg_pane_latency = 0; // average pane latency (usec) over the entire execution
	// statistics of a print_sampling interval
	unsigned long panes_sample = 0; // number of completed panes per print_sampling interval
	unsigned long ins_sample = 0; // number of completed pane partitions per print_sampling interval
	double avg_ins_size = 0; // average size of the results set of the pane partitions measured during a print_sampling interval
	double Sn_ins = 0; // n*\sigma^{2}_{n} of the size of the results set of the pane partitions measured during a print_sampling interval
	double avg_pane_size = 0; // average number of tuples selected per pane measured during a print_sampling interval
	double avg_closing_delay = 0; // average closing delay (usec) of panes measured during a print_sampling interval
	double avg_pane_latency = 0; // average pane latency (usec) of panes measured during a print_sampling interval
#if defined(LOG)
	vector<pair<double, double>> client_plq_stat_vect; // when all the results sets of a pane are complete, we save the arrival time to the client and the latency
	// create the statistics log file of the client_plq
	ofstream client_plq_logfile;
	ostringstream client_plq_logstream;
#endif

public:
	// constructor
	Client_PLQ(): cnt_ins(RESERVE_V_SPACE, -1), cnt_ins_rcv(RESERVE_V_SPACE, 0), sizes_pane_vect(RESERVE_V_SPACE, 0), starting_pane_vect(RESERVE_V_SPACE, numeric_limits<size_t>::max()), ending_pane_vect(RESERVE_V_SPACE, 0), closing_pane_vect(RESERVE_V_SPACE, 0) {}

	// destructor
	~Client_PLQ() {}

	// svc_init method
	int svc_init() {
#if defined(LOG)
		// open the statistics log file of the client_plq
		client_plq_logfile.open(CLIENT_PLQ_LOG_FILENAME);
		client_plq_logstream << "#second results avg_latency(ms)\n";
#endif
		// set the thread mapping onto a SMT context
		ff_mapThreadToCpu(CLIENT_CORE_ID);
		// entering the sampling barrier
		pthread_barrier_wait(&sampleBarr);
		start_ticks = getticks();
		last_print = start_ticks;
		cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << 0.0 << ", Started!" << endl;
		return 0;
	}

	// svc_end method
	void svc_end() {
		// print statistics of the last results (if there are any)
		volatile ticks end_ticks = getticks();
		if(panes_sample > 0) {
			double time = ((double)(end_ticks-start_ticks)/((long long)((unsigned long)(FREQ)*1000000))); // in seconds
			cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << time << ", recv panes [" << CYAN << setw(3) << panes_sample << RESET << "], results (pane ins.) [avg: " << setw(8) <<  avg_ins_size << "; std: " << setw(8) << sqrt(Sn_ins/ins_sample) << "], results (pane) [avg: " << setw(8) << avg_pane_size << "], closing delay [avg: " << setw(10) << avg_closing_delay << " us], latency [avg: " << setw(10) << avg_pane_latency << " us]" << endl;
		}
		// compute the rate of panes computed per second
		pane_rate = ((double) rcv_panes) / (FROM_TICKS_TO_USECS(end_ticks - start_ticks) / 1000000);
		// entering the printing barrier
		pthread_barrier_wait(&printBarr);
		// entering the precedence barrier 1
		pthread_barrier_wait(&precBarr1);
		// entering the precedence barrier 2
		pthread_barrier_wait(&precBarr2);
		MY_PRINT("*******************************Final statistics*******************************\n" <<\
			      "Received: " << pane_rate << " panes/sec, total " << rcv_panes << " panes\n" <<\
			      "Average closing delay per pane: " << tot_avg_closing_delay << " us\n" <<\
			      "Average latency per pane: " << tot_avg_pane_latency/1000 << " ms\n" <<\
			      "Results per pane: avg " << tot_avg_pane_size << ", std " << sqrt(tot_Sn_pane_size/rcv_panes) << "\n"\
			      "******************************************************************************\n");
 #if defined(LOG)
 		// print the final statistics of the client_plq in its log file
 		size_t elapsed_sec = 0;
 		size_t results_sec = 0;
 		double avg_lat_sec = 0;
 		double time_threshold_usec = 1000000;
 		for(pair<double, double> &r: client_plq_stat_vect) {
 			if(r.first <= time_threshold_usec) {
 				results_sec++;
 				avg_lat_sec += (1/((double) results_sec)) * (r.second - avg_lat_sec);
 			}
 			else {
 				// update the log statistics per second seen by the client_plq
				client_plq_logstream << elapsed_sec << " " << results_sec << " " << avg_lat_sec/1000 << "\n";
				elapsed_sec++;
				time_threshold_usec += 1000000;
				while(r.first > time_threshold_usec) {
					// update the log statistics per second seen by the client_plq
					client_plq_logstream << elapsed_sec << " " << 0 << " " << 0 << "\n";
					elapsed_sec++;
					time_threshold_usec += 1000000;
				}
				results_sec = 1;
				avg_lat_sec = (1/((double) results_sec)) * (r.second);
 			}
 		}
 		// update the log statistics per second seen by the client_plq
		client_plq_logstream << elapsed_sec << " " << results_sec << " " << avg_lat_sec/1000 << "\n";
		// close the statistics log file of the client_plq
		client_plq_logfile << client_plq_logstream.str();
 		client_plq_logfile.close();
#endif
	}

	// svc method
	Rset_t *svc(Rset_t *r) {
		// increase the number of pane partitions
		ins_sample++;
		// increase the vectors if needed
		size_t pane_id = r->getId();
		if(pane_id >= cnt_ins.size()) {
			rcv_instances++;
			cnt_ins.resize(pane_id+1, -1);
			cnt_ins_rcv.resize(pane_id+1, 0);
			sizes_pane_vect.resize(pane_id+1, 0);
			starting_pane_vect.resize(pane_id+1, numeric_limits<size_t>::max());
			ending_pane_vect.resize(pane_id+1, 0);
			closing_pane_vect.resize(pane_id+1, 0);
		}
		// if the total number of partitions of pane_id is not known
		if(cnt_ins[pane_id] == -1 && r->getNoInstances() > 0) {
			cnt_ins[pane_id] = r->getNoInstances();
			avg_ins_per_pane += (1/((double) rcv_panes+1)) * (r->getNoInstances() - avg_ins_per_pane);
		}
		cnt_ins_rcv[pane_id]++; // we have received the results set of another pane partition
		// update the size of pane_id
		sizes_pane_vect[pane_id] += r->getSize();
		// update the starting time of the pane
		if(starting_pane_vect[pane_id] > FROM_TICKS_TO_USECS(r->getStartingTime()))
			starting_pane_vect[pane_id] = FROM_TICKS_TO_USECS(r->getStartingTime());
		// update the ending time of the pane
		if(ending_pane_vect[pane_id] < FROM_TICKS_TO_USECS(r->getEndingTime()))
			ending_pane_vect[pane_id] = FROM_TICKS_TO_USECS(r->getEndingTime());
		// update the closing time of the pane
		if(closing_pane_vect[pane_id] < FROM_TICKS_TO_USECS(r->getClosingTime()))
			closing_pane_vect[pane_id] = FROM_TICKS_TO_USECS(r->getClosingTime());
		// statistics of the size of the results set of the pane partition
		double old_avg = avg_ins_size;
		avg_ins_size += (1/((double) ins_sample)) * (r->getSize() - avg_ins_size);
		Sn_ins += (r->getSize() - old_avg) * (r->getSize() - avg_ins_size);
		// a pane is complete when we have received all the results sets of its partitions
		if(cnt_ins[pane_id] == cnt_ins_rcv[pane_id]) {
			rcv_panes++;
			panes_sample++;
			// compute the average latency of the panes
			double latency = closing_pane_vect[pane_id] - starting_pane_vect[pane_id];
			avg_pane_latency += (1/((double) panes_sample)) * (latency - avg_pane_latency);
			// compute the total average latency of panes
			tot_avg_pane_latency += (1/((double) rcv_panes)) * (latency - tot_avg_pane_latency);	
			// compute the average delay of the panes
			double closing_delay = closing_pane_vect[pane_id] - ending_pane_vect[pane_id];
			avg_closing_delay += (1/((double) panes_sample)) * (closing_delay - avg_closing_delay);
			// compute the total average closing delay of panes
			tot_avg_closing_delay += (1/((double) rcv_panes)) * (closing_delay - tot_avg_closing_delay);
			// compute the average number of selected tuples per pane
			avg_pane_size += (1/((double) panes_sample)) * (sizes_pane_vect[pane_id] - avg_pane_size);
			// compute the total average number of selected tuples per pane
			double old_tot_avg = tot_avg_pane_size;
			tot_avg_pane_size += (1/((double) rcv_panes)) * (sizes_pane_vect[pane_id] - tot_avg_pane_size);
			tot_Sn_pane_size += (sizes_pane_vect[pane_id] - old_tot_avg) * (sizes_pane_vect[pane_id] - tot_avg_pane_size);
			// statistics per second seen by the client_plq
			double arrival_time = FROM_TICKS_TO_USECS(getticks() - start_ticks);
			client_plq_stat_vect.push_back(pair<double, double>(arrival_time, latency));
		}
		volatile double elapsed = FROM_TICKS_TO_USECS(getticks()-last_print);
		if(elapsed > PRINT_RATE_USEC) {
			double time = ((double)(getticks()-start_ticks)/((long long)((unsigned long)(FREQ)*1000000))); // in seconds
			cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << time << ", recv panes [" << CYAN << setw(3) << panes_sample << RESET << "], results (x partition) [avg: " << setw(8) <<  avg_ins_size << "; std: " << setw(8) << sqrt(Sn_ins/ins_sample) << "], results (x pane) [avg: " << setw(8) << avg_pane_size << "], closing delay [avg: " << setw(10) << avg_closing_delay << " us], latency [avg: " << setw(10) << avg_pane_latency << " us]" << endl;
			panes_sample = avg_ins_size = Sn_ins = avg_closing_delay = avg_pane_latency = ins_sample = avg_pane_size = 0;
			last_print = getticks();
		}
		delete r;
		return ff_minode_t<Rset_t>::GO_ON;
	}
};

/*! 
 *  \class Client
 *  
 *  \brief Client class of the Pane Farming
 *  
 *  Class that implements the generic client that receives results set of completed
 *  windows from the Pane Farming pattern.
 *  
 *  This class is defined in \ref Pane_Farming/include/pf_query.cpp
 */
class Client: public ff_node_t<Rset_t> {
private:
	volatile ticks last_print, start_ticks;
	unsigned long rcv_windows = 0; // number of received results sets of windows over the entire execution
	double avg_latency = 0; // window average latency (usec) over the entire execution
	double win_rate = 0; // number of windows completed per second over the entire execution
	double tot_avg_win_results = 0; // average size of the windows results set over the entire execution
	vector<size_t> win_result_sizes; // win_sizes[i] contains the size of the results set of the i-th window
	size_t next_win_id = 0; // identifier of the next window to receive
	// statistics of a print_sampling interval
	unsigned long wins_sample = 0; // number of completed windows during a print_sampling interval
	double avg_win_results = 0; // average size of the window results set measured during a print_sampling interval
	double Sn = 0; // n*\sigma^{2}_{n} of the size of the window results set measured during a print_sampling interval
	double avg_latency_sample = 0; // window average latency (usec) measured during a print_sampling interval
#if defined(LOG)
	vector<pair<double, double>> client_stat_vect; // for each window results set we save the arrival time to the client and its latency
	// create the statistics log file of the client
	ofstream client_logfile;
	ostringstream client_logstream;
#endif

public:
	// constructor
	Client() {}

	// destructor
	~Client() {}

	// svc_init method
	int svc_init() {
#if defined(LOG)
		// open the statistics log file of the client
		client_logfile.open(CLIENT_LOG_FILENAME);
		client_logstream << "#second results avg_latency(ms)\n";
#endif
		// set the thread mapping onto a SMT context
		ff_mapThreadToCpu(CLIENT_CORE_ID);
		// entering the sampling barrier
		pthread_barrier_wait(&sampleBarr);
		start_ticks = getticks();
		last_print = start_ticks;
		cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << 0.0 << ", Started!" << endl;
		return 0;
	}

	// svc_end method
	void svc_end() {
		// print statistics of the last results (if there are any)
		volatile ticks end_ticks = getticks();
		if(wins_sample > 0) {
			double time = ((double)(end_ticks-start_ticks)/((long long)((unsigned long)(FREQ)*1000000))); // in seconds
			cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << time << ", recv wins [" << CYAN << setw(3) << wins_sample << RESET << "], results (win) [avg: " << setw(8) <<  avg_win_results << "; std: " << setw(8) << sqrt(Sn/wins_sample) << "], latency [avg: " << setw(12) << avg_latency_sample << " us]" << endl;
		}
		// compute the rate of wins computed per second
		win_rate = ((double) rcv_windows) / (FROM_TICKS_TO_USECS(end_ticks - start_ticks) / 1000000);
		// entering the printing barrier
		pthread_barrier_wait(&printBarr);
		// entering the precedence barrier 1
		pthread_barrier_wait(&precBarr1);
		// entering the precedence barrier 2
		pthread_barrier_wait(&precBarr2);
		// entering the precedence barrier 3
		pthread_barrier_wait(&precBarr3);
		MY_PRINT("*******************************Final statistics*******************************\n" <<\
			      "Received: " << win_rate << " wins/sec, total " << rcv_windows << " wins\n" <<\
			      "Average latency per window: " << avg_latency/1000 << " ms\n" <<\
			      "Average no. of results per window: " << tot_avg_win_results << "\n" <<\
			      "******************************************************************************\n");
		// create, open, update and close the results log file of the client 
		ofstream results_logfile;
		results_logfile.open(RESULTS_LOG_FILENAME);
		ostringstream results_logstream;
 		results_logstream << "**************** Completed windows and their result size ****************\n";
 		for(size_t i=0; i<win_result_sizes.size(); i++)
 			results_logstream << "Window " << i << " with " << win_result_sizes[i] << " result points\n";
 		results_logfile << results_logstream.str();
 		results_logfile.close();
 #if defined(LOG)
 		// print the final statistics of the client in its log file
 		size_t elapsed_sec = 0;
 		size_t results_sec = 0;
 		double avg_lat_sec = 0;
 		double time_threshold_usec = 1000000;
 		for(pair<double, double> &r: client_stat_vect) {
 			if(r.first <= time_threshold_usec) {
 				results_sec++;
 				avg_lat_sec += (1/((double) results_sec)) * (r.second - avg_lat_sec);
 			}
 			else {
 				// update the log statistics per second seen by the client
				client_logstream << elapsed_sec << " " << results_sec << " " << avg_lat_sec/1000 << "\n";
				elapsed_sec++;
				time_threshold_usec += 1000000;
				while(r.first > time_threshold_usec) {
					// update the log statistics per second seen by the client
					client_logstream << elapsed_sec << " " << 0 << " " << 0 << "\n";
					elapsed_sec++;
					time_threshold_usec += 1000000;
				}
				results_sec = 1;
				avg_lat_sec = (1/((double) results_sec)) * (r.second);
 			}
 		}
 		// update the log statistics per second seen by the client
		client_logstream << elapsed_sec << " " << results_sec << " " << avg_lat_sec/1000 << "\n";
		// close the statistics log file of the client
		client_logfile << client_logstream.str();
 		client_logfile.close();
#endif
	}

	// svc method
	Rset_t *svc(Rset_t *w) {
#if !defined(UNORDERING_COLLECTOR)
		// check the result ordering that should be preserved by the WLQ collector
		if(next_win_id != w->getId()) {
			cout << "Window " << w->getId() << " received out-of-order!" << endl;
		}
		else next_win_id++;
#endif
		// increase the number of window completed
		rcv_windows++;
		wins_sample++;
		// statistics per second seen by the client
		volatile double latency = FROM_TICKS_TO_USECS(getticks() - w->getStartingTime());
		double arrival_time = FROM_TICKS_TO_USECS(getticks() - start_ticks);
		client_stat_vect.push_back(pair<double, double>(arrival_time, latency));
		// statistics per print_sampling interval
		double old_avg = avg_win_results;
		avg_win_results += (1/((double) wins_sample)) * (w->getSize() - avg_win_results);
		Sn += (w->getSize() - old_avg) * (w->getSize() - avg_win_results);
		avg_latency_sample += (1/((double) wins_sample)) * (latency - avg_latency_sample);
		avg_latency += (1/((double) rcv_windows)) * (latency - avg_latency);
		tot_avg_win_results += (1/((double) rcv_windows)) * (w->getSize() - tot_avg_win_results);
		double elapsed = FROM_TICKS_TO_USECS(getticks()-last_print);
		if(elapsed > PRINT_RATE_USEC) {
			double time = ((double)(getticks()-start_ticks)/((long long)((unsigned long)(FREQ)*1000000))); // in seconds
			cout << "[CLIENT] Time: " << fixed << setprecision(2) << setw(7) << time << ", recv wins [" << CYAN << setw(3) << wins_sample << RESET << "], results (x win) [avg: " << setw(8) <<  avg_win_results << "; std: " << setw(8) << sqrt(Sn/wins_sample) << "], latency [avg: " << setw(12) << avg_latency_sample << " us]" << endl;
			wins_sample = avg_win_results = Sn = 0;
			last_print = getticks();
		}
		// update the final statistics in the result file
		if(w->getId() >= win_result_sizes.size())
			win_result_sizes.resize(w->getId()+1, 0);
		win_result_sizes[w->getId()] = w->getSize();
		delete w;
		return ff_node_t<Rset_t>::GO_ON;
	}
};

// main
int main(int argc, char *argv[]) {
	int option = 0;
	size_t port = 10000;
	double drop_prob = 0;
	unsigned long win_len = 0; // window length (in ms)
	unsigned long win_slide = 0; // window slide (in ms)
	size_t nw_plq = 0; // parallelism degree of the PLQ stage
	size_t nw_wlq = 0; // parallelism degree of the WLQ stage
#if defined(PLQ_ONLY)
	// arguments from command line
	if(argc != 11) {
		cout << argv[0] << " -p <port> -d <dropping probability> -w <win length ms> -s <win slide ms> -n <PLQ workers>" << endl;
		exit(0);
	}
	while ((option = getopt(argc, argv, "p:d:w:s:n:")) != -1) {
		switch (option) {
			case 'p': port = atoi(optarg);
				break;
			case 'd': drop_prob = stod(optarg);
				break;
			case 'w': win_len = atoi(optarg);
				break;
			case 's': win_slide = atoi(optarg);
				break;
			case 'n': {
				nw_plq = atoi(optarg);
				if(nw_plq < 1) {
					cout << "The parallelism degree must be at least 1" << endl;
					exit(0);
				}
				break;
			}
			default: {
				cout << argv[0] << " -p <port> -d <dropping probability> -w <win length ms> -s <win slide ms> -n <PLQ workers>" << endl;
				exit(0);
			}
        }
    }
#else
	// arguments from command line
	if(argc != 13) {
		cout << argv[0] << " -p <port> -d <dropping probability> -w <win length ms> -s <win slide ms> -n <PLQ workers> -m <WLQ workers>" << endl;
		exit(0);
	}
	while ((option = getopt(argc, argv, "p:d:w:s:n:m:")) != -1) {
		switch (option) {
			case 'p': port = atoi(optarg);
				break;
			case 'd': drop_prob = stod(optarg);
				break;
			case 'w': win_len = atoi(optarg);
				break;
			case 's': win_slide = atoi(optarg);
				break;
			case 'n': {
				nw_plq = atoi(optarg);
				if(nw_plq < 1) {
					cout << "The parallelism degree (PLQ) must be at least 1" << endl;
					exit(0);
				}
				break;
			}
			case 'm': {
				nw_wlq = atoi(optarg);
				if(nw_wlq < 1) {
					cout << "The parallelism degree (WLQ) must be at least 1" << endl;
					exit(0);
				}
				break;
			}
			default: {
				cout << argv[0] << " -p <port> -d <dropping probability> -w <win length ms> -s <win slide ms> -n <PLQ workers> -m <WLQ workers>" << endl;
				exit(0);
			}
        }
    }
#endif
    // check the total number of workers (PLQ+WLQ)
    if((nw_plq + nw_wlq) > NUM_WORKER_THREADS) {
    	cout << "Error: too many workers!" << endl;
    	exit(1);
    }
    cout << "The frequency of the CPU is " << FREQ << " Mhz" << endl;
#if defined(ADAPTIVE_SCHED_PID)
	cout << "Insert the PLQ utilization factor setpoint [close to 1]: ";
	cin >> desired_rho; // desired_rho is a global variable
	cout << "Chosen setpoint is: " << desired_rho << endl;
#endif
    // if LOG mode is enabled we initialize the log files
#if defined(LOG)
    LOG_PLQ_INIT();
#if !defined(PLQ_ONLY)
    LOG_WLQ_INIT();
#endif
#endif
    // create a pipeline with unbounded queues
    ff::ff_pipeline pipe;
#if defined(PLQ_ONLY)
	// initialize the sampling barrier between PLQ emitter, controller and client_PLQ
	pthread_barrier_init(&sampleBarr, NULL, 3);
	// initialize the printing barrier between PLQ emitter, controller and client_PLQ
	pthread_barrier_init(&printBarr, NULL, 3);
	// initialize the precedence barrier 1 between PLQ emitter, controller and client_PLQ
	pthread_barrier_init(&precBarr1, NULL, 3);
	// initialize the precedence barrier 2 between PLQ emitter, controller and client_PLQ
	pthread_barrier_init(&precBarr2, NULL, 3);
    PF<Rset_t> pf(win_len, win_slide, drop_prob, port, nw_plq);
    pipe.add_stage(&pf); // add the PF pattern to the pipeline
    Client_PLQ client;
    pipe.add_stage(&client); // add the client node to the pipeline
#else
	// initialize the sampling barrier between PLQ emitter, WLQ emitter, controller and client
	pthread_barrier_init(&sampleBarr, NULL, 4);
	// initialize the printing barrier between PLQ emitter, WLQ emitter, controller and client
	pthread_barrier_init(&printBarr, NULL, 4);
	// initialize the precedence barrier 1 between PLQ emitter, WLQ emitter, controller and client_PLQ
	pthread_barrier_init(&precBarr1, NULL, 4);
	// initialize the precedence barrier 2 between PLQ emitter, WLQ emitter, controller and client_PLQ
	pthread_barrier_init(&precBarr2, NULL, 4);
	// initialize the precedence barrier 3 between PLQ emitter, WLQ emitter, controller and client_PLQ
	pthread_barrier_init(&precBarr3, NULL, 4);
    PF<Rset_t> pf(win_len, win_slide, drop_prob, port, nw_plq, nw_wlq);
    pipe.add_stage(&pf); // add the PF pattern to the pipeline
    Client client;
    pipe.add_stage(&client); // add the client node to the pipeline
#endif
    pipe.setFixedSize(false); // the pipeline uses unbounded queues!
#if defined(SKYLINE)
    cout << "Starting pipeline (Skyline Query)..." << endl;
#elif defined(TOPD)
    cout << "Starting pipeline (Top-δ Dominant Skyline Query)..." << endl;
#endif
#if defined(ENERGY)
    Mammut m;
    Energy *energy = m.getInstanceEnergy();
    Joules j;
    energy_counter = energy->getCounter(); // energy_counter is a global variable
    if(!energy_counter) {
        cout << "Error: power counters are not available on this machine!" << endl;
        exit(1);
    }
    energy_counter->reset();
#endif
    pipe.run_then_freeze();
    pipe.wait_freezing();
    if(pipe.wait() < 0) {
    	cerr << "Error executing the pipeline" << endl;
    	return -1;
    }
    else {
    	cout << "...done" << endl;
    }
#if defined(ENERGY)
    j = energy_counter->getJoules();
    cout << "Consumed joules are: " << j << endl;
#endif
    // if LOG mode is enabled we close the log files
#if defined(LOG)
    LOG_PLQ_CLOSE();
#if !defined(PLQ_ONLY)
    LOG_WLQ_CLOSE();
#endif
#endif
    //pipe.ffStats(cout);
    return 0;
}
