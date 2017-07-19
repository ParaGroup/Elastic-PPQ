/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file queries.hpp
 *  \brief Implementation of the Supported Continuous Preference Queries
 *  
 *  In this file we provide a set of classes for supporting various kinds of
 *  continuous preference queries. To date, the supported queries are:
 *  1- Skyline query: it returns the results set of non-dominated points;
 *  2- Top-δ dominant skyline query: it returns the results set of the first
 *     δ>=1 points with the lowest dominance factor, where the dominance
 *     factor of a point p is the maximum k in [0, DIM] such that there exists
 *     a p' that k-dominates p (it is equal or better than p in at least k
 *     dimensions and better in at least one of these dimensions).
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

#ifndef QUERIES_H
#define QUERIES_H

// include
#include <vector>
#include <Rset.hpp>
#include <general.hpp>
#include <tuple_t.hpp>

using namespace std;

/* 
 * This file provides the following classes:
 * -TData: class implementing a special container of tuples (column-based layout);
 * -SkySet: class implementing the results set for skyline queries;
 * -TopdSet: class implementing the results set for top-δ dominant skyline query.
 */

/*! 
 *  \class TData
 *  
 *  \brief Efficient Container of Tuples
 *  
 *  Container for the efficient storage of tuples for the query processing.
 *  It uses a column-oriented memory layout.
 *  
 *  This class is defined in \ref Pane_Farming/include/queries.hpp
 */
class TData {
public:
	// index of the last valid tuple (-1 the container is empty)
	int last_idx = -1;
	// vectors for the tuples' coordinates + timestamps
	std::vector<double> v_dim[DIM+2];

    // constructor
    TData() {
        // reserve space for the vectors
        for(size_t d=0; d<DIM+2; d++) v_dim[d].reserve(RESERVE_V_SPACE);
    }

    // destructor
    ~TData() {}

    // method to append a tuple at the end of the container
    inline void append(tuple_t const &t) {
        last_idx++;
        for(size_t d=0; d<DIM; d++) v_dim[d].push_back(t.d[d]);
        v_dim[DIM].push_back(t.app_ts);
        v_dim[DIM+1].push_back(t.gen_ts);
    }

    // method to remove all the tuples from position pos (included) to the end of the container
    inline void removeFromIndex(size_t pos) {
        // if position is not valid return;
        if(pos > last_idx) return;
        // if the tuple to remove is the last one
        if(pos == last_idx) {
            for(size_t d=0; d<DIM+2; d++) v_dim[d].pop_back();
        }
        else {
            // maintain only the tuples from the one with index 0 to pos-1
            for(size_t d=0; d<DIM+2; d++) v_dim[d].resize(pos);
        }
        last_idx=pos-1;
    }

	// method to return the number of tuples in the container
	inline size_t getSize() const {
		return (size_t) (last_idx+1);
	}

	// copy operator
	TData& operator=(const TData &other) {
		this->last_idx = other.last_idx;
		for(int i=0; i<DIM+2; i++) this->v_dim[i] = other.v_dim[i];
		return *this;
	}
};

/*! 
 *  \class SkySet
 *  
 *  \brief Results Set Implementation for Skyline Queries
 *  
 *  Results set implementation for continuous skyline queries.
 *  
 *  This class is defined in \ref Pane_Farming/include/queries.hpp
 */
class SkySet: public Rset {
private:
	// pointer to the tuples container
	TData *s;

public:
	// constructor
	SkySet(size_t _refCounter=1, size_t _id=0, size_t _n_instances=1): Rset(_refCounter, _id, _n_instances) {
		// create the TData container
		s = new TData();
	}

	// destructor
	~SkySet() {
		delete s;
	}

	/* 
	 * Method to update the results set by eventually adding a new tuple
	 * to it and eventually deleting existing tuples from the results set.
	 */
	inline void addComputeTuple(tuple_t &t) {
		// update the starting time
		if(this->getStartingTime() == 0) this->setStartingTime(t.arrival_ticks);
		else if(this->getStartingTime() > t.arrival_ticks) this->setStartingTime(t.arrival_ticks);
		int pos;
		int endpoint = this->getSize() - 1;
		bool isDominated = updateSkyline(t, pos, endpoint);
		// pos is the index of the last point in the container not dominated by t
		if(pos < s->last_idx) {
			// remove all the points in the container dominated by t
			s->removeFromIndex(pos+1);
		}
		// if t does not dominate any point in the container and it is not dominated, we append it to the container
		if((pos == s->last_idx) && !isDominated) s->append(t);
		// update the ending time
		this->setEndingTime(getticks());
	}

	/* 
	 * This method merges two results sets this and B into one results set of the
	 * most preferred tuples by taking into account all the tuples in this and B. The
	 * new results set is computed by updating the tuples in this which is properly
	 * modified.
	 */
	inline void addComputeSet(Rset &B) {
		// update the starting time
		if(this->getStartingTime() == 0) this->setStartingTime(B.getStartingTime());
		else if(B.getStartingTime() > 0) {
			if(B.getStartingTime() < this->getStartingTime()) this->setStartingTime(B.getStartingTime());
		}
		SkySet *BB = static_cast<SkySet *>(&B);
		// if this is empty, all the tuples in B are copied in this
		if(this->getSize() == 0) {
			// insert all the tuples in B in this
			TData *l_sky = BB->getData(); // skyline of the results set B
			*s = *l_sky; // copy the container of B into this
		}
		// otherwise we update the results set this by trying to add the tuples in B
		else {
			TData *l_sky = BB->getData(); // skyline of the results set B
			int endpoint = this->getSize() - 1;
			for(int i=0; i<=l_sky->last_idx; i++) {
				// create the tuple
				tuple_t t;
				for(size_t d=0; d<DIM; d++) t.d[d] = l_sky->v_dim[d][i];
				t.app_ts = l_sky->v_dim[DIM][i];
				t.gen_ts = l_sky->v_dim[DIM+1][i];
				int pos;
				bool isDominated = updateSkyline(t, pos, endpoint);
				// pos is the index of the last point in the container not dominated by t
				if(pos < s->last_idx) {
					// remove all the points in the container dominated by t
					s->removeFromIndex(pos+1);
				}
				// if t does not dominate any point in the container and it is not dominated, we append it to the container
				if((pos == s->last_idx) && !isDominated) s->append(t);
			}
		}
		// update the ending time
		this->setEndingTime(getticks());
	}

	/* 
	 * Method to shift all the tuples dominated by t at the end of the container. The index
	 * of the last tuple not dominated by t (-1 if all the elements are dominated) is written
	 * in the input argument pos passed by reference. The method also returns whether the point
	 * t is dominated by any point in the container or not (return value).
	 * Worst case complexity: O(n*d).
	 */
	inline bool updateSkyline(tuple_t const &t, int &pos, int &endpoint) {
		// if Tdom[i] >= 1, the i-th point is dominated by t and can be shifted
		int Tdom[endpoint+1];
		// if dom[i] >= 1, the i-th point dominates t
		int domT[endpoint+1];
		// flag is true if t is dominated by at least one point in the container
		bool isDominated = false;
		// first dimension d=0
		size_t d = 0;
		for(int i=0; i<=endpoint; i++) {
			Tdom[i] = (s->v_dim[d][i] > t.d[d]) - (DIM+1)*(s->v_dim[d][i] < t.d[d]);
			domT[i] = (s->v_dim[d][i] < t.d[d]) - (DIM+1)*(s->v_dim[d][i] > t.d[d]);
		}
		// dimensions from d=1 to d=DIM-2
		for(d=1; d <= DIM-2; d++) {
			for(int i=0; i<=endpoint; i++) {
				Tdom[i] += (s->v_dim[d][i] > t.d[d]) - (DIM+1)*(s->v_dim[d][i] < t.d[d]);
				domT[i] += (s->v_dim[d][i] < t.d[d]) - (DIM+1)*(s->v_dim[d][i] > t.d[d]);
			}
		}
		// last dimension d=DIM-1
		for(int i=0; i<=endpoint; i++) {
			Tdom[i] += (s->v_dim[d][i] > t.d[d]) - (DIM+1)*(s->v_dim[d][i] < t.d[d]);
			domT[i] += domT[i] += (s->v_dim[d][i] < t.d[d]) - (DIM+1)*(s->v_dim[d][i] > t.d[d]);
			if(domT[i] >= 1) isDominated = true;
		}
		// scan all the tuples and shift the dominated ones to the end of the container
		int i = 0;
		pos = s->last_idx;
		while(i <= endpoint) {
			// if the i-th tuple is dominated by t
			if(Tdom[i] >= 1) {
				// copy the tuple in position endpoint in the position i
				for(size_t d=0; d<DIM+2; d++) s->v_dim[d][i] = s->v_dim[d][endpoint];
				Tdom[i] = Tdom[endpoint];
				domT[i] = domT[endpoint];
				// copy the tuple in position pos in position endpoint
				for(size_t d=0; d<DIM+2; d++) s->v_dim[d][endpoint] = s->v_dim[d][pos];
				endpoint--;
				pos--;
			}
			else i++;
		}
		return isDominated;
	}

	// method to return the number of tuples in the results set
	inline size_t getSize() const {
		return s->getSize();
	}

	// method to return the pointer to the container of the results set
	inline TData *getData() const {
		return s;
	}

	// method to transform the skyline tuples of the results set into a vector of plain tuples
	vector<tuple_t> *toVector() const {
		vector<tuple_t> *result_vect = new vector<tuple_t>(s->last_idx+1);
		// for all the tuples in the container
		for(int i=0; i<=s->last_idx; i++) {
			// for all the dimensions and timestamps
			for(size_t d=0; d<DIM; d++) (*result_vect)[i].d[d] = s->v_dim[d][i];
			(*result_vect)[i].app_ts = s->v_dim[DIM][i];
			(*result_vect)[i].gen_ts = s->v_dim[DIM+1][i];
		}
		return result_vect;
	}
};

/*! 
 *  \class TopdSet
 *  
 *  \brief Results Set Implementation for Top-δ dominant skyline query
 *  
 *  Results set implementation for continuous top-δ dominant skyline query.
 *  
 *  This class is defined in \ref Pane_Farming/include/queries.hpp
 */
class TopdSet: public Rset {
private:
	// pointer to the tuples container
	TData *s;
	// vectors of the dominance factors of the tuples in the container
	vector<size_t> domFactors;

public:
	// constructor
	TopdSet(size_t _refCounter=1, size_t _id=0, size_t _n_instances=0): Rset(_refCounter, _id, _n_instances) {
		// create the TData container
		s = new TData();
		domFactors.reserve(RESERVE_V_SPACE);
	}

	// destructor
	~TopdSet() {
		delete s;
	}

	/* 
	 * Method to update the results set by eventually adding a new tuple
	 * to it and eventually deleting existing tuples from the results set.
	 */
	inline void addComputeTuple(tuple_t &t) {
		// set the starting time of the results set
		if(this->getStartingTime() == 0) this->setStartingTime(t.arrival_ticks);
		else if(this->getStartingTime() > t.arrival_ticks) this->setStartingTime(t.arrival_ticks);
		int pos;
		size_t dom = updateTopd(t, pos);
		if(pos < s->last_idx) {
			s->removeFromIndex(pos+1);
			domFactors.resize(pos+1);
		}
		if(dom < DIM) {
			// t is a skyline point, we append it in the container
			s->append(t);
			domFactors.push_back(dom);
		}
		// set the ending time of the pane
		this->setEndingTime(getticks());
	}

	/* 
	 * This method merges two results sets this and B into one results set of the
	 * most preferred tuples by taking into account all the tuples in this and B. The
	 * new results set is computed by updating the tuples in this which is properly
	 * modified.
	 */
	inline void addComputeSet(Rset &B) {
		// update the starting time
		if(this->getStartingTime() == 0) this->setStartingTime(B.getStartingTime());
		else if(B.getStartingTime() > 0) {
			if(B.getStartingTime() < this->getStartingTime()) this->setStartingTime(B.getStartingTime());
		}
		TopdSet *BB = static_cast<TopdSet *>(&B);
		// if this is empty, all the tuples in B are copied in this
		if(this->getSize() == 0) {
			// insert all the tuples in B in this
			TData *l_sky = BB->getData(); // skyline of the results set B
			*s = *l_sky; // copy the container of B into this
			domFactors = BB->getDomVector(); // copy the vector of dominance factors from B to this		
		}
		// otherwise we update the results set this by trying to add the tuples in B
		else {
			TData *l_sky = BB->getData(); // skyline of the input results set B
			vector<size_t> const &doms = BB->getDomVector(); // dominance vector of the results set B
			for(int i=0; i<=l_sky->last_idx; i++) {
				// create the tuple
				tuple_t t;
				for(size_t d=0; d<DIM; d++) t.d[d] = l_sky->v_dim[d][i];
				t.app_ts = l_sky->v_dim[DIM][i];
				t.gen_ts = l_sky->v_dim[DIM+1][i];
				int pos;
				size_t dom = updateTopd(t, pos);
				if(pos < s->last_idx) {
					s->removeFromIndex(pos+1);
					domFactors.resize(pos+1);
				}
				if(dom < DIM) {
					// t is a skyline point, we append it in the container
					s->append(t);
					domFactors.push_back(max(dom, doms[i]));
				}
			}
		}
		// update the ending time
		this->setEndingTime(getticks());
	}

	/* 
	 * Method to return the dominance factor of a tuple t given the tuples stored in
	 * the container of the results set. This method also updates the dominance factor
	 * of all the tuples in the container. The points with dominance factor equal to DIM
	 * (non skyline points) are shifted in the last positions. The index of the last tuple
	 * with dominance factor < DIM is written in the input argument pos passed by reference.
	 * Worst case complexity: O(n*d).
	 */
	inline size_t updateTopd(tuple_t const &t, int &pos) {
		size_t domFactor = 0; // dominance factor of t
		int domT[s->last_idx+1]; // domT[i] = K if the i-th tuple is equal/better than t in exactly K dimensions
		int noBetterT[s->last_idx+1];
		// at the end all the tuples with id in [0, idx] will be the ones not dominated by t
		int idx = s->last_idx;
		int Tdom[s->last_idx+1]; // dom[i] = K if t is equal/better than the i-th tuple in exactly K dimensions
		int TnoBetter[s->last_idx+1];
		// first dimension d=0
		size_t d = 0;
		for(int i=0; i<=s->last_idx; i++) {
			domT[i] = (s->v_dim[d][i] <= t.d[d]);
			noBetterT[i] = !(s->v_dim[d][i] <= t.d[d]) || (s->v_dim[d][i] == t.d[d]);
			Tdom[i] = (t.d[d] <= s->v_dim[d][i]);
			TnoBetter[i] = !(t.d[d] <= s->v_dim[d][i]) || (t.d[d] == s->v_dim[d][i]);
		}
		// dimensions from d=1 to d=DIM-2
		for(d=1; d <= DIM-2; d++) {
			for(int i=0; i<=s->last_idx; i++) {
				domT[i] += (s->v_dim[d][i] <= t.d[d]);
				noBetterT[i] *= !(s->v_dim[d][i] <= t.d[d]) || (s->v_dim[d][i] == t.d[d]);
				Tdom[i] += (t.d[d] <= s->v_dim[d][i]);
				TnoBetter[i] = !(t.d[d] <= s->v_dim[d][i]) || (t.d[d] == s->v_dim[d][i]);
			}
		}
		// last dimension d=DIM-1
		for(int i=0; i<=s->last_idx; i++) {
			domT[i] += (s->v_dim[d][i] <= t.d[d]);
			noBetterT[i] *= !(s->v_dim[d][i] <= t.d[d]) || (s->v_dim[d][i] == t.d[d]);
			domT[i] *= (1 - noBetterT[i]);
			// update the dominance factor of t
			if(domT[i] > domFactor) domFactor = domT[i];
			Tdom[i] += (t.d[d] <= s->v_dim[d][i]);
			TnoBetter[i] = !(t.d[d] <= s->v_dim[d][i]) || (t.d[d] == s->v_dim[d][i]);
			Tdom[i] *= (1 - TnoBetter[i]);
			if(Tdom[i] > domFactors[i]) domFactors[i] = Tdom[i];
		}
		// scan all the tuples and shift the dominated ones to the end of the container
		int i = 0;
		while(i <= idx) {
			// if the i-th tuple is dominated by t
			if(domFactors[i] == DIM) {
				// switch the tuple with the one in the last position
				for(size_t d=0; d<DIM+2; d++) s->v_dim[d][i] = s->v_dim[d][idx];
				domFactors[i] = domFactors[idx];
				idx--;
			}
			else i++;
		}
		pos = idx;
		return domFactor;
	}

	// method to return the number of tuples in the results set
	inline size_t getSize() const {
		return s->getSize();
	}

	// method to return the pointer to the container of the results set
	inline TData *getData() const {
		return s;
	}

	// method to return the vector of dominance factors of the tuples in the container
	inline vector<size_t> const &getDomVector() const {
		return domFactors;
	}

	// method to transform the top-δ tuples of the results set into a vector of tuples
	vector<tuple_t> *toVector() const {
		// determine the number of tuples per dominance factor
		size_t n_tuples[DIM+1]; // n_tuples[i] = no. of tuples with dominance factor i
		fill_n(n_tuples, DIM+1, 0);
		for(size_t i=0; i<=s->last_idx; i++) {
			n_tuples[domFactors[i]]++;
		}
		/* 
		 * Determine the smallest dominance factor kdom for which
	   	 * there exist at least δ=TOPD_NUMBER tuples with dominance
	   	 * factor lower or equal than kdom.
	   	 */
		size_t kdom = 0;
		size_t needed_tuples = TOPD_NUMBER; // we use δ=TOPD_NUMBER
		size_t counters[DIM];
		fill_n(counters, DIM, 0);
		while((kdom < DIM) && (needed_tuples > 0)) {
			if(needed_tuples <= n_tuples[kdom]) {
				counters[kdom] = needed_tuples;
				needed_tuples = 0;
			}
			else {
				needed_tuples -= n_tuples[kdom];
				counters[kdom] = n_tuples[kdom];
				kdom++;
			}
		}
		// create the final vector
		needed_tuples = 0;
		vector<tuple_t> *result = new vector<tuple_t>();
		for(int i=0; i<=s->last_idx; i++) {
			if(counters[domFactors[i]] > 0) {
				counters[domFactors[i]]--;
				needed_tuples++;
				// create the tuple
				tuple_t t;
				for(size_t d=0; d<DIM; d++) t.d[d] = s->v_dim[d][i];
				t.app_ts = s->v_dim[DIM][i];
				t.gen_ts = s->v_dim[DIM+1][i];
				result->push_back(t);
				if(needed_tuples >= TOPD_NUMBER) break;
			}
		}
		return result;
	}
};

#endif
