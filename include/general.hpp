/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file general.hpp
 *  \brief General Definitions and Macros
 *  
 *  This file contains a set of useful definitions and macros used by other
 *  files of the project.
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
 * March 2016
 */

#ifndef _GENERAL_H
#define _GENERAL_H

// include
#include <mutex>
#include <vector>
#include <fstream>
#include <iostream>
#include <ff/cycle.h>
#if defined(ENERGY)
 	#include <mammut/mammut.hpp>
	using namespace mammut;
	using namespace mammut::energy;
#endif

using namespace std;

// architectural defines (add a define block for each machine)
#if (defined(PIANOSA) || defined(PIANOSAU))
	#define FREQ 2000 // frequency Mhz
	#define CORE_OFFSET 1 // offset to pass to the the next core indentifier
	#define CONTEXT_OFFSET 16 // offset to pass to the next SMT context indentifier on the same core
	#define GEN_CORE_ID 0
	#define PLQ_EMITTER_CORE_ID 1
	#define CLIENT_CORE_ID 2
	#define CONTROLLER_CORE_ID 18 // client and controller are in two SMT contexts of the same core
	#define WLQ_EMITTER_CORE_ID 3
	#define WLQ_COLLECTOR_CORE_ID 4
#if defined(PLQ_ONLY)
	#define NUM_WORKER_CORES 13 // number of cores dedicated to execute PLQ worker threads
	#define NUM_WORKER_THREADS 13 // number of workers threads to instantiate in the PLQ (max 26)
#else
	#define NUM_WORKER_CORES 11 // number of cores dedicated to execute PLQ and WLQ worker threads
	#define NUM_WORKER_THREADS 11 // number of workers threads to instantiate in the PLQ and in the WLQ (max 22)
#endif
#endif

#if defined(REPARA)
	#define FREQ 2400 // frequency Mhz
	#define CORE_OFFSET 1 // offset to pass to the the next core indentifier
	#define CONTEXT_OFFSET 24 // offset to pass to the next SMT context indentifier on the same core
	#define GEN_CORE_ID 0
	#define PLQ_EMITTER_CORE_ID 1
	#define CLIENT_CORE_ID 2
	#define CONTROLLER_CORE_ID 26 // client and controller are in two SMT contexts of the same core
	#define WLQ_EMITTER_CORE_ID 3
	#define WLQ_COLLECTOR_CORE_ID 4
#if defined(PLQ_ONLY)
	#define NUM_WORKER_CORES 21 // number of cores dedicated to execute PLQ worker threads
	#define NUM_WORKER_THREADS 21 // number of workers threads to instantiate in the PLQ (max 42)
#else
	#define NUM_WORKER_CORES 19 // number of cores dedicated to execute PLQ and WLQ worker threads
	#define NUM_WORKER_THREADS 19 // number of workers threads to instantiate in the PLQ and in the WLQ (max 38)
#endif
#endif

#if defined(REPHRASE)
	#define FREQ 512 // frequency Mhz. It is a magic value, trust me!
	#define CORE_OFFSET 8 // offset to pass to the the next core indentifier
	#define CONTEXT_OFFSET 1 // offset to pass to the next SMT context indentifier on the same core
	#define GEN_CORE_ID 0
	#define PLQ_EMITTER_CORE_ID 8
	#define CLIENT_CORE_ID 16
	#define CONTROLLER_CORE_ID 17 // client and controller are in two SMT contexts of the same core
	#define WLQ_EMITTER_CORE_ID 24
	#define WLQ_COLLECTOR_CORE_ID 32
#if defined(PLQ_ONLY)
	#define NUM_WORKER_CORES 17 // number of cores dedicated to execute PLQ worker threads
	#define NUM_WORKER_THREADS 17 // number of workers threads to instantiate in the PLQ (max 136)
#else
	#define NUM_WORKER_CORES 15 // number of cores dedicated to execute PLQ and WLQ worker threads
	#define NUM_WORKER_THREADS 15 // number of workers threads to instantiate in the PLQ and in the WLQ (max 120)
#endif
#endif

#if defined(NINJA)
	#define FREQ 1300 // frequency Mhz
	#define CORE_OFFSET 1 // offset to pass to the the next core indentifier
	#define CONTEXT_OFFSET 64 // offset to pass to the next SMT context indentifier on the same core
	#define GEN_CORE_ID 0
	#define PLQ_EMITTER_CORE_ID 1
	#define CLIENT_CORE_ID 2
	#define CONTROLLER_CORE_ID 66 // client and controller are in two SMT contexts of the same core
	#define WLQ_EMITTER_CORE_ID 3
	#define WLQ_COLLECTOR_CORE_ID 4
#if defined(PLQ_ONLY)
	#define NUM_WORKER_CORES 61 // number of cores dedicated to execute PLQ worker threads
	#define NUM_WORKER_THREADS 61 // number of workers threads to instantiate in the PLQ (max 244)
#else
	#define NUM_WORKER_CORES 59 // number of cores dedicated to execute PLQ and WLQ worker threads
	#define NUM_WORKER_THREADS 59 // number of workers threads to instantiate in the PLQ and in the WLQ (max 236)
#endif
#endif

// generic defines
#define FROM_TICKS_TO_USECS(ticks) (((double) ticks)/FREQ)
#define FROM_USECS_TO_TICKS(usecs) (ticks) (FREQ * usecs)
#define SAMPLING_KSLACK_USEC 100000 // time interval used by the Adaptive K-slack buffering/punctuation mechanism
#define MAX_TIME_BETWEEN_PUNC_USEC 10000 // maximum time interval between two consecutive punctuations (used to limit the no. of punctuations)
#define SAMPLING_ASPID_PLQ_USEC 250000 // time interval used by the AS_PID scheduling strategy in the PLQ
#define CONTROL_STEP_USEC 2500000 // time interval of a control step used by the elastic controller
#define PRINT_RATE_USEC 10000000 // time interval between two consecutive prints by the client
#define RESERVE_V_SPACE 500 // pre-allocated size of vectors and deques in the whole project
#define TICKS2WAIT 10000 // number of ticks of a busy-waiting phase
#define TOPD_NUMBER 100 // number of δ>0 tuples to produce in the top-δ dominant skyline query
#define PLQ_WORKERS_LOG_FILENAME "log/workers_plq.log" // filename of the PLQ workers log file (written if the macro LOG is enabled)
#define WLQ_WORKERS_LOG_FILENAME "log/workers_wlq.log" // filename of the WLQ workers log file (written if the macro LOG is enabled)
#define CONTROLLER_LOG_FILENAME "log/controller.log" // filename of the controller log file (written if the macro LOG is enabled)
#define CLIENT_LOG_FILENAME "log/client.log" // filename of the client log file (written if the macro LOG is enabled)
#define CLIENT_PLQ_LOG_FILENAME "log/client_plq.log" // filename of the client_plq log file (written if the macro LOG is enabled)
#define RESULTS_LOG_FILENAME "log/results.log" // filename with the size of the window results collected by the client (to check the program correctness)

// Linux terminal colors
#define RESET 		"\033[0m"  // to reset the color to the standard one
#define BLACK 		"\033[30m"
#define RED   		"\033[31m"
#define GREEN   	"\033[32m"
#define YELLOW  	"\033[33m"
#define BLUE    	"\033[34m"
#define MAGENTA 	"\033[35m"
#define CYAN    	"\033[36m"
#define WHITE   	"\033[37m"
#define BOLDBLACK   "\033[1m\033[30m"
#define BOLDRED     "\033[1m\033[31m"
#define BOLDGREEN   "\033[1m\033[32m"
#define BOLDYELLOW  "\033[1m\033[33m"
#define BOLDBLUE    "\033[1m\033[34m"
#define BOLDMAGENTA "\033[1m\033[35m"
#define BOLDCYAN    "\033[1m\033[36m"
#define BOLDWHITE   "\033[1m\033[37m"

// global variable and define to atomically write a string in the stdout
std::mutex mutex_screen; // mutex to avoid mixing couts

#define MY_PRINT(...) { \
 	std::ostringstream stream; \
 	stream << __VA_ARGS__; \
 	mutex_screen.lock(); \
	std::cout << stream.str(); \
	mutex_screen.unlock(); \
}

// set of global variables and defines useful for logging purposes
std::mutex mutex_log_plq; // mutex to access the log file of the PLQ workers
std::mutex mutex_log_wlq; // mutex to access the log file of the WLQ workers
std::ofstream logfile_plq; // log file of the PLQ workers
std::ofstream logfile_wlq; // log file of the WLQ workers

// main thread must call this macro (PLQ)
#define LOG_PLQ_INIT() { \
	logfile_plq.open(PLQ_WORKERS_LOG_FILENAME); \
}

// main thread must call this macro (PLQ)
#define LOG_PLQ_CLOSE() { \
	logfile_plq.close(); \
}

// macro to write atomically in the log file (PLQ)
#define LOG_PLQ_WRITE(stream) { \
	mutex_log_plq.lock(); \
	logfile_plq << stream.str(); \
	mutex_log_plq.unlock(); \
}

// main thread must call this macro (WLQ)
#define LOG_WLQ_INIT() { \
	logfile_wlq.open(WLQ_WORKERS_LOG_FILENAME); \
}

// main thread must call this macro (WLQ)
#define LOG_WLQ_CLOSE() { \
	logfile_wlq.close(); \
}

// macro to write atomically in the log file (WLQ)
#define LOG_WLQ_WRITE(stream) { \
	mutex_log_wlq.lock(); \
	logfile_wlq << stream.str(); \
	mutex_log_wlq.unlock(); \
}

// global barrier to synchronize the sampling in all the entities of the implementation
pthread_barrier_t sampleBarr;

// global barrier to avoid interleaved final prints among the implementation entities
pthread_barrier_t printBarr;

// global barriers to print the statistics in a given order among the various implementation entities
pthread_barrier_t precBarr1;
pthread_barrier_t precBarr2;
pthread_barrier_t precBarr3;

// global variable with the setpoint of the PLQ utilization factor (used if ADAPTIVE_SCHED_PID is enabled)
double desired_rho;

#if defined(ENERGY)
Counter *energy_counter; // global variable of the mammut counter to obtain the joules consumed by the pane farming pattern
#endif

// struct of the monitored data gathered by the Controller from the PLQ Emitter
struct PLQ_Monitored_Data {
	double rho; // PLQ utilization factor
	vector<size_t> split_vect; // vector of the number of partitions of the completed panes

	// constructor
	PLQ_Monitored_Data(double _rho, vector<size_t> _split_vect): rho(_rho), split_vect(_split_vect) {}

	// destructor
	~PLQ_Monitored_Data() {}
};

// struct of the monitored data gathered by the Controller from the WLQ Emitter
struct WLQ_Monitored_Data {
	double rho; // WLQ utilization factor
	double idle; // fraction passed in idle mode by the WLQ workers
	size_t cnt_win_tasks; // number of WIN_UPDATE WLQ tasks scheduled
	size_t cnt_merge_tasks; // number of MERGE WLQ tasks scheduled

	// constructor
	WLQ_Monitored_Data(double _rho, double _idle, size_t _cnt_win, size_t _cnt_merge): rho(_rho), idle(_idle), cnt_win_tasks(_cnt_win), cnt_merge_tasks(_cnt_merge) {}

	// destructor
	~WLQ_Monitored_Data() {}
};

/* 
 * Enumeration of the possible reconfiguration message types:
 * -ADD = add a certain number of workers to the destination stage;
 * -REMOVE = remove a certain number of workers from the destination stage;
 * -NORECONF = no reconfiguration is triggered.
 */
enum type_reconf_t { ADD, REMOVE, NORECONF };

// struct of a reconfiguration message
struct Reconf_t {
	type_reconf_t type; // type of the reconfiguration
	vector<size_t> w_ids; // identifiers of the workers involved in the reconfiguration

	// constructor I
	Reconf_t(): type(NORECONF) {}

	// constructor II
	Reconf_t(type_reconf_t _type, vector<size_t> _w_ids): type(_type), w_ids(_w_ids) {}

	// destructor
	~Reconf_t() {}
};

#endif
