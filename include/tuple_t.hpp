/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file tuple_t.hpp
 *  \brief Tuple Message
 *  
 *  This file provides the implementation of the generic message exchanged
 *  between the emitter and the workers of the PLQ stage of Pane Farming.
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

#ifndef _TUPLE_H
#define _TUPLE_H

// include
#include <string.h>
#include <atomic>
#include <sstream>
#include <iostream>
#include <general.hpp>

using namespace std;

/* 
 * This file provides the following class:
 * -tuple_t: generic message exchanged between the emitter and the workers of the PLQ stage.
 */

/* 
 * Enumeration of the possible message types:
 * -UNSPECIFIED = the message type is not specified;
 * -REGULAR = a message of this type is a regular tuple;
 * -FLUSH = a message of this type, transmitted to some PLQ workers (MULTICAST), will empty their pane deques;
 * -NEW_INST = a message of this type is used to inform that a pane has a further instance, it is transmitted to some PLQ workers (MULTICAST);
 * -PUNCT = a message of this type is a punctuation transmitted to the PLQ workers (MULTICAST);
 * -LULL = a message of this type is used to create a closed empty pane in the destination worker;
 * -EMPTYPANE = a message of this type is used to create an empty pane (that can be filled in the future) in the destination worker;
 * -END = a message of this type is an end-of-stream message transmitted to the PLQ workers (MULTICAST).
 */
enum type_msg_t { UNSPECIFIED, REGULAR, FLUSH, NEW_INST, PUNCT, LULL, EMPTYPANE, END };

/*! 
 *  \class tuple_t
 *  
 *  \brief Tuple Message
 *  
 *  This file provides the implementation of the generic message exchanged
 *  between the emitter and the workers of the PLQ stage of Pane Farming.
 *  
 *  This class is defined in \ref Pane_Farming/include/tuple_t.hpp
 */
class tuple_t {
public:
    type_msg_t type; // type of the message
    atomic_size_t refCounter; // number of workers that will receive this message (meaningful only for MULTICAST messages)
    double punc_value; // punctuation value (meaningful if type == PUNCT)
    size_t no_instances; // number of instances of pane_id (meaningful if type == NEW_INST, REGULAR, LULL, EMPTYPANE)
    size_t pane_id; // not meaningful if type is UNSPECIFIED, FLUSH or END
    // fields meaningful if type == REGULAR
	double d[DIM]; // array of DIM attributes
	double app_ts; // application timestamp (in usecs)
	double gen_ts; // generator timestamp (in usecs)
    volatile ticks arrival_ticks; // arrival time of the tuple (in ticks)

    // empty constructor
    tuple_t() {
        type = UNSPECIFIED;
        punc_value = 0;
        no_instances = 0;
        refCounter = 0;
        pane_id = 0;
        fill_n(d, DIM, 0);
        app_ts = 0;
        gen_ts = 0;
        arrival_ticks = 0;
    }

    // copy constructor
    tuple_t(tuple_t const &t) {
        type = t.type;
        punc_value = t.punc_value;
        no_instances = t.no_instances;
        refCounter.store((t.refCounter).load());
        pane_id = t.pane_id;
        memcpy(d, t.d, sizeof(double)*DIM);
        app_ts = t.app_ts;
        gen_ts = t.gen_ts;
        arrival_ticks = t.arrival_ticks;
    }

    // copy operator
    tuple_t &operator= (const tuple_t &other) {
        type = other.type;
        punc_value = other.punc_value;
        no_instances = other.no_instances;
        refCounter.store((other.refCounter).load());
        pane_id = other.pane_id;
        memcpy(d, other.d, sizeof(double)*DIM);
        app_ts = other.app_ts;
        gen_ts = other.gen_ts;
        arrival_ticks = other.arrival_ticks;
        return *this;
    }

    // != operator (using only the DIM dimensions of regular tuples)
    bool operator!=(const tuple_t &other) {
        for(size_t i=0; i<DIM; i++) {
            if(d[i] != other.d[i]) return true;
        }
        return false;
    }

    // < operator (using only the generator timestamp of regular tuples)
    bool operator<(const tuple_t &other) {
        if(gen_ts < other.gen_ts) return true;
        else return false;
    }

    // get the size of the tuple's fields in bytes (only for regular tuples)
    static inline size_t getSize() {
        // only the fields d, app_ts, gen_ts
        return sizeof(double)*(DIM+2);
    }

    // serialization method (only for regular tuples)
    inline int serialize(char *buffer) {
        if(type != REGULAR) abort();
    	char *data = (char *) d;
    	int b = 0;
    	for(size_t i = 0; i<DIM; i++) {
    		for(size_t j = 0; j<sizeof(double); j++) buffer[b++] = data[(i*sizeof(double))+j];
    	}
    	data = (char *) &app_ts;
    	for(size_t j = 0; j<sizeof(double); j++) buffer[b++] = data[j];
    	data = (char *) &gen_ts;
    	for(size_t j = 0; j<sizeof(double); j++) buffer[b++] = data[j];
        return 0;
    }

    // method to use the tuple as a regular tuple
    inline int create_regular(char *buffer, size_t pane_length) {
        type = REGULAR;
    	char *data = (char *) d;
    	int b = 0;
    	for(size_t i=0; i<DIM; i++) {
    		for(size_t j=0; j<sizeof(double); j++) data[(i*sizeof(double))+j] = buffer[b++];
    	}
    	data = (char *) &app_ts;
    	for(size_t j=0; j<sizeof(double); j++) data[j] = buffer[b++];
    	data = (char *) &gen_ts;
    	for(size_t j=0; j<sizeof(double); j++) data[j] = buffer[b++];
        pane_id = (size_t) (app_ts / (pane_length * 1000)); // set the pane identifier of the tuple
        no_instances =  1; // by default we have at least one instance per pane
        arrival_ticks = getticks(); // set the arrival time of the tuple in ticks
        return 0;
    }

    // method to use the tuple as a punctuation message
    inline void create_punctuation(double ts, size_t p_id, size_t nw) {
        type = PUNCT;
        punc_value = ts; // punctuation value
        pane_id = p_id; // pane id corresponding to the punctuation value
        refCounter = nw; // number of receivers of the punctuation
    }

    // method to use the tuple as a END message
    inline void create_end(size_t nw) {
        type = END;
        refCounter = nw; // number of receivers of the END message
    }

    // method to use the tuple as a NEW_INST message
    inline void create_newinst(size_t p_id, size_t no_inst, size_t nw) {
        type = NEW_INST;
        pane_id = p_id; // pane id
        no_instances = no_inst; // new number of instances of pane id
        refCounter = nw; // number of receivers of the message
    }

    // method to use the tuple as a FLUSH message
    inline void create_flush(size_t nw, size_t p_id=0) {
        type = FLUSH;
        pane_id = p_id;
        refCounter = nw; // number of receivers of the message
    }

    // method to use the tuple as a LULL message
    inline void create_lull(size_t p_id) {
        type = LULL;
        pane_id = p_id;
        no_instances = 1;
    }

    // method to use the tuple as an EMPTYPANE message
    inline void create_emptypane(size_t p_id, size_t no_inst) {
        type = EMPTYPANE;
        pane_id = p_id;
        no_instances = no_inst;
    }

    // print the tuple content (the DIM dimensions of a regular tuple)
    void print() const {
        for(size_t i=0; i<DIM-1; i++) {
            cout << d[i] << " ";
        }
        cout << d[DIM-1] << endl;
    }
};

#endif
