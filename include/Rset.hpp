/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file Rset.hpp
 *  \brief Results Set (set of preferred tuples)
 *  
 *  A results set (Rset briefly) is a set of preferred tuples according to
 *  the criterion utilized by the preference query. For example, it is the
 *  skyline set in the skyline query. Depending on where this set is
 *  computed, it can represent the set of preferred tuples within the temporal
 *  range of a pane (or a pane partition in case of splitting), or the results
 *  set of a whole time-based window.
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

#ifndef PANE_H
#define PANE_H

// include
#include <atomic>
#include <general.hpp>
#include <tuple_t.hpp>

using namespace std;

/* 
 * This file provides the following class:
 * -Rset: class of a results set. It must be extened to implement a specific preference query.
 */

/*! 
 *  \class Rset
 *  
 *  \brief Abstract Class of a Results Set
 *  
 *  This class must be extended to provide the specific implementation of the
 *  results set for a desired query executed by the Pane Farming pattern.
 *  
 *  This class is defined in \ref Pane_Farming/include/Rset.hpp
 */
class Rset {
protected:
	// atomic counter: the results set is destroyed when refCounter reaches zero
	std::atomic_size_t refCounter;
	// identifier of the results set (starting from 0)
	size_t id;
	// counter of the number of results sets existing with the same id (0 undefined)
	size_t no_instances;
	// starting time (in ticks)
	volatile ticks starting_time = 0;
	// ending time (in ticks)
	volatile ticks ending_time = 0;
	// closing time (in ticks)
	volatile ticks closing_time = 0;

	// constructor (it is protected, the class cannot be instantiated directly)
	Rset(size_t _refCounter=1, size_t _id=0, size_t _no_instances=1): refCounter(_refCounter), id(_id), no_instances(_no_instances) {}

public:
	// method to update the results set with a new tuple
	virtual void addComputeTuple(tuple_t &t) = 0;
	// method to update the results set with a new results set
	virtual void addComputeSet(Rset &rset) = 0;
	// method to return the actual results set cardinality
	virtual size_t getSize() const = 0;
	// method to return the identifier of the results set (starting from 0)
	inline size_t getId() const { return id; }
	// method to return the number of existing instances of the results set
	inline size_t getNoInstances() const { return no_instances; }
	// method to return the starting time (in ticks)
	inline ticks getStartingTime() const { return starting_time; }
	// method to return the ending time (in ticks)
	inline ticks getEndingTime() const { return ending_time; }
	// method to return the closing time (in ticks)
	inline ticks getClosingTime() const { return closing_time; }
	// method to decrease the reference counter of the results set and atomically return the past value
	inline size_t decreaseRefCounter() { return refCounter.fetch_sub(1); }
	// method to set the identifier of the results set (starting from 0)
	inline void setId(size_t _id) { id = _id; }
	// method to set the number of existing instances of the results set
	inline void setNoInstances(size_t _no_instances) { no_instances = _no_instances; }
	// method to set the starting time (in ticks)
	inline void setStartingTime(ticks _starting_time) { starting_time = _starting_time; }
	// method to set the ending time (in ticks)
	inline void setEndingTime(ticks _ending_time) { ending_time = _ending_time; }
	// method to set the closing time (in ticks)
	inline void setClosingTime(ticks _closing_time) { closing_time = _closing_time; }
};

#endif
