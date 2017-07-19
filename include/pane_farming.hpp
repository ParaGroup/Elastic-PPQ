/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file pane_farming.hpp
 *  \brief Pane Farming
 *  
 *  Implementation of the Pane Farming parallel pattern. It is implemented as
 *  a pipeline of two stages, each one potentially parallel (farm). The first
 *  stage is called Pane-level Query (PLQ) and processes each pane independently
 *  (a pane is a tumbling window of tuples). Once processed, results sets of the
 *  panes are transmitted to a second stage called Window-level Query (WLQ) that
 *  merges all the results sets of the panes belonging to the same window into
 *  a final results set of the window. Results sets of panes in common to more
 *  than one window can be re-used and not computed multiple times.
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

#ifndef PF_H
#define PF_H

// include
#include <algorithm>
#include <functional>
#include <ff/farm.hpp>
#include <ff/pipeline.hpp>
#include <plq.hpp>
#include <wlq.hpp>
#include <queries.hpp>
#include <controller.hpp>

using namespace ff;
using namespace std;

/* 
 * This file provides the following class:
 * -PF: class that implements the Pane Farming.
 */

/*! 
 *  \class PF
 *  
 *  \brief Pane Farming
 *  
 *  Pane Farming is a parallel pattern for window-based streaming computations.
 *  It consists in a pipeline of two stages (PLQ and WLQ). The first processes
 *  panes. The second works on count-based sliding windows of computed panes
 *  and merges them by producing a final result for each window.
 *  
 *  This class is defined in \ref Pane_Farming/include/pane_farming.hpp
 */
#if defined(PLQ_ONLY) // <----- version with the PLQ stage only!
template<typename Rset_t>
class PF: public ff_pipeline {
private:
#if !defined(IOP)
    typedef PLQ_Emitter plq_emitter_t;
    typedef PLQ_Worker<Rset_t> plq_worker_t;
#else
    typedef PLQ_Emitter_IOP plq_emitter_t;
    typedef PLQ_Worker_IOP<Rset_t> plq_worker_t;
#endif
	size_t nw_plq; // parallelism degree of the PLQ
    plq_emitter_t *plq_emitter; // pointer to the emitter of the first stage (PLQ)
    vector<ff_node *> plq_w_vect; // vector of pointers to workers of the first stage (PLQ)
    ff_farm<> *plq_farm; // pointer to the first stage (PLQ)
    ff_buffernode *ch_plq; // pointer to a fastflow channel from the elastic controller to the PLQ stage
    Controller<plq_emitter_t, plq_emitter_t> *controller; // elastic controller of the PLQ stage

public:
    /* 
     * \brief constructor
     * 
     * \param _wlen window length (in ms)
     * \param _wslide window slide (in ms)
     * \param _drop_prob desired dropping probability (0 uses the standard slack mechanism)
     * \param _port port to receive connection from the Generator
     * \param _nw_plq parallelism degree of the PLQ stage
     * 
     */
    PF(size_t _wlen, size_t _wslide, double _drop_prob, size_t _port, size_t _nw_plq): ff_pipeline(), nw_plq(_nw_plq) {
        if(_wlen < _wslide) {
            cout << "Error: window length must be greater than the slide parameter!" << endl;
            exit(1);
        }
        // calculate the length in ms of the pane
        auto gcd = [](size_t u, size_t v) {
            while(v != 0) {
                unsigned long r = u % v;
                u = v;
                v = r;
            }
            return u;
        };
        size_t pane_len = gcd(_wlen, _wslide);
        // calculate the window parameters in terms of panes
        size_t wp = _wlen / pane_len; // window length (in no. panes)
        size_t sp = _wslide / pane_len; // window slide (in no. panes)
        cout << "Window parameters [wp: " << wp << "; sp: " << sp << "; pane length: " << pane_len << " ms], dimensions per tuple: " << DIM << endl;
        // we create the pattern PLQ_Emitter->{PLQ_Workers}
        size_t i = 0;
        size_t start_idx = CLIENT_CORE_ID + CORE_OFFSET;
        int totW = NUM_WORKER_THREADS;
        while(i <= (NUM_WORKER_THREADS/NUM_WORKER_CORES)) {
            for(size_t j=0; j<min(totW, NUM_WORKER_CORES); j++) {
                size_t cpuid = start_idx + (i*CONTEXT_OFFSET) + (j*CORE_OFFSET);
                plq_w_vect.push_back(new plq_worker_t(wp, sp, pane_len, cpuid));
            }
            totW = totW - min(totW, NUM_WORKER_CORES);
            i++;
        }
        ch_plq = new ff_buffernode(100, true, MAX_NUM_THREADS+100);
        plq_farm = new ff_farm<>();
        plq_farm->add_workers(plq_w_vect);
        plq_emitter = new plq_emitter_t(pane_len, nw_plq, _port, _drop_prob, plq_farm->getlb(), ch_plq);
        plq_farm->add_emitter(plq_emitter);
        plq_farm->remove_collector();
        this->add_stage(plq_farm);
        this->setFixedSize(false); // everything uses bounded queues now!
        controller = new Controller<plq_emitter_t, plq_emitter_t>(plq_farm->getlb(), plq_emitter, ch_plq, plq_farm->getlb(), plq_emitter, ch_plq, nw_plq, 0 /*nw_wlq*/);
    }

    // destructor
    ~PF() {
        delete ch_plq;
        delete plq_emitter;
        for(auto const &node: plq_w_vect) {
            plq_worker_t *w = static_cast<plq_worker_t *>(node);
            delete w;
        }
        delete plq_farm;
        delete controller;
    }

    // freeze_and_run method
    int freeze_and_run(bool skip_init=false) {
        controller->run();
        return ff_pipeline::freeze_and_run(skip_init);
    }

    // wait method
    int wait() {
        controller->wait();
        return ff_pipeline::wait();
    }
};
#else // <----- complete version of the pattern
template<typename Rset_t>
class PF: public ff_pipeline {
private:
#if !defined(IOP)
    typedef PLQ_Emitter plq_emitter_t;
    typedef PLQ_Worker<Rset_t> plq_worker_t;
#else
    typedef PLQ_Emitter_IOP plq_emitter_t;
    typedef PLQ_Worker_IOP<Rset_t> plq_worker_t;
#endif
    typedef WLQ_Emitter<Rset_t> wlq_emitter_t;
    typedef WLQ_Worker<Rset_t> wlq_worker_t;
    typedef WLQ_Collector<Rset_t> wlq_collector_t;
    size_t nw_plq; // parallelism degree of the PLQ
    size_t nw_wlq; // parallelism degree of the WLQ
    plq_emitter_t *plq_emitter; // pointer to the emitter of the first stage (PLQ)
    vector<ff_node *> plq_w_vect; // vector of pointers to workers of the first stage (PLQ)
    wlq_emitter_t *wlq_emitter; // pointer to the emitter of the second stage (WLQ)
    vector<ff_node *> wlq_w_vect; // vector of pointers to workers of the second stage (WLQ)
    wlq_collector_t *wlq_collector; // pointer to the collector of the second stage (WLQ)
    ff_farm<> *plq_farm; // pointer to the first stage (PLQ)
    ff_farm<> *wlq_farm; // pointer to the second stage (WLQ)
    ff_buffernode *ch_plq; // pointer to a fastflow channel from the elastic controller to the PLQ stage
    ff_buffernode *ch_wlq; // pointer to a fastflow channel from the elastic controller to the WLQ stage
    Controller<plq_emitter_t, wlq_emitter_t> *controller; // elastic controller of the Pane Farming pattern

public:
    /* 
     * \brief constructor
     * 
     * \param _wlen window length (in ms)
     * \param _wslide window slide (in ms)
     * \param _drop_prob desired dropping probability (0 uses the base slack mechanism)
     * \param _port port to receive connection from the Generator
     * \param _nw_plq parallelism degree of the PLQ stage
     * \param _nw_wlq parallelism degree of the WLQ stage
     * 
     */
    PF(size_t _wlen, size_t _wslide, double _drop_prob, size_t _port, size_t _nw_plq, size_t _nw_wlq): ff_pipeline(), nw_plq(_nw_plq), nw_wlq(_nw_wlq) {
        if(_wlen < _wslide) {
            cout << "Error: window length must be greater than the slide parameter!" << endl;
            exit(1);
        }
        // calculate the length in ms of the pane
        auto gcd = [](size_t u, size_t v) {
            while(v != 0) {
                unsigned long r = u % v;
                u = v;
                v = r;
            }
            return u;
        };
        size_t pane_len = gcd(_wlen, _wslide);
        // calculate the window parameters in terms of panes
        size_t wp = _wlen / pane_len; // window length (in no. panes)
        size_t sp = _wslide / pane_len; // window slide (in no. panes)
        cout << "Window parameters [wp: " << wp << "; sp: " << sp << "; pane length: " << pane_len << " ms], dimensions per tuple: " << DIM << endl;
        // we create the pattern PLQ_Emitter->{PLQ_Workers}->WLQ_Emitter->{WLQ_Workers}->WLQ_Collector
        size_t i = 0;
        size_t start_idx = WLQ_COLLECTOR_CORE_ID + CORE_OFFSET;
        int totW = NUM_WORKER_THREADS;
        while(i <= (NUM_WORKER_THREADS/NUM_WORKER_CORES)) {
            for(size_t j=0; j<min(totW, NUM_WORKER_CORES); j++) {
                size_t cpuid = start_idx + (i*CONTEXT_OFFSET) + (j*CORE_OFFSET);
                plq_w_vect.push_back(new plq_worker_t(wp, sp, pane_len, cpuid));
                wlq_w_vect.push_back(new wlq_worker_t(cpuid));
            }
            totW = totW - min(totW, NUM_WORKER_CORES);
            i++;
        }
        ch_plq = new ff_buffernode(100, true, MAX_NUM_THREADS+100);
        ch_wlq = new ff_buffernode(100, true, MAX_NUM_THREADS+200);
        plq_farm = new ff_farm<>();
        plq_farm->add_workers(plq_w_vect);
        plq_emitter = new plq_emitter_t(pane_len, nw_plq, _port, _drop_prob, plq_farm->getlb(), ch_plq);
        plq_farm->add_emitter(plq_emitter);
        plq_farm->remove_collector();
        this->add_stage(plq_farm);
        wlq_farm = new ff_farm<>();
        wlq_farm->add_workers(wlq_w_vect);
        (wlq_farm->getlb())->addManagerChannel(ch_wlq);
        wlq_emitter = new wlq_emitter_t(wp, sp, nw_wlq, wlq_farm->getlb());
        wlq_farm->add_emitter(wlq_emitter);
        wlq_farm->remove_collector();
        wlq_farm->setMultiInput(); // the second farm receives directly from the PLQ workers
        wlq_farm->wrap_around(); // feedback channels
        wlq_collector = new wlq_collector_t();
        wlq_farm->add_collector(wlq_collector);
        this->add_stage(wlq_farm);
        this->setFixedSize(false); // everything uses bounded queues now!
        controller = new Controller<plq_emitter_t, wlq_emitter_t>(plq_farm->getlb(), plq_emitter, ch_plq, wlq_farm->getlb(), wlq_emitter, ch_wlq, nw_plq, nw_wlq);
    }

    // destructor
    ~PF() {
        delete ch_plq;
        delete ch_wlq;
        delete plq_emitter;
        for(auto const &node: plq_w_vect) {
            plq_worker_t *w = static_cast<plq_worker_t *>(node);
            delete w;
        }
        delete plq_farm;
        delete wlq_emitter;
        for(auto const &node: wlq_w_vect) {
            wlq_worker_t *w = static_cast<wlq_worker_t *>(node);
            delete w;
        }
        delete wlq_collector;
        delete wlq_farm;
        delete controller;
    }

    // freeze_and_run method
    int freeze_and_run(bool skip_init=false) {
        controller->run();
        return ff_pipeline::freeze_and_run(skip_init);
    }

    // wait method
    int wait() {
        controller->wait();
        return ff_pipeline::wait();
    }
};
#endif

#endif
