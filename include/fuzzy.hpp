/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*! 
 *  \file fuzzy.hpp
 *  \brief Adaptation Strategy based on a Fuzzy Control System
 *  
 *  This file provides the implementation of an adaptation strategy of the Pane
 *  Farming controller based on a Fuzzy Control System (FCS).
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

#ifndef FUZZY_H
#define FUZZY_H

// include
#include <math.h>
#include "fl/Headers.h" // fuzzylite library headers
#include <general.hpp>
#include <controller.hpp>

using namespace fl;
using namespace std;

/* 
 * This file provides the following classes:
 # -FuzzyStrategy: adaptation strategy based on a FCS.
 */

/*! 
 *  \class FuzzyStrategy
 *  
 *  \brief Adaptation Strategy based on a Fuzzy Control System
 *  
 *  Class implementing an adaptation strategy of the Pane Farming controller
 *  based on a Fuzzy Control System.
 *  
 *  This class is defined in \ref Pane_Farming/include/fuzzy.hpp
 */
class FuzzyStrategy {
private:
	Engine *engine; // engine of the FCS
	InputVariable *plq_rho_in; // linguistic input variable of the PLQ utilization factor
	InputVariable *plq_sigma_in; // linguistic input variable of the PLQ splitting factor
	InputVariable *wlq_rho_in; // linguistic input variable of the WLQ utilization factor
	OutputVariable *plq_degree_var_out; // linguistic output variable of the PLQ parallelism degree variation
	OutputVariable *wlq_degree_var_out; // linguistic output variable of the WLQ parallelism degree variation
	RuleBlock *ruleBlock; // block of fuzzy-logic rules

public:
	// constructor
	FuzzyStrategy(): engine(new Engine("PF_Fuzzy_Strategy")) {
		// *****************************************START INITIALIZATION OF THE FUZZY ADAPTATION STRATEGY***************************************** //
		// initialization of the input linguistic variables
		plq_rho_in = new InputVariable();
		plq_rho_in->setEnabled(true);
		plq_rho_in->setName("PLQ_RHO");
		plq_rho_in->setRange(0.000, 2.000);
		plq_rho_in->addTerm(new Trapezoid("FAST", 0.000, 0.000, 0.500, 0.900));
		plq_rho_in->addTerm(new Triangle("ACCEPTABLE", 0.500, 0.900, 1.300));
		plq_rho_in->addTerm(new Trapezoid("SLOW", 0.900, 1.300, 2.000, 2.000));
		engine->addInputVariable(plq_rho_in);
		plq_sigma_in = new InputVariable();
		plq_sigma_in->setEnabled(true);
		plq_sigma_in->setName("PLQ_SPLITTING");
		plq_sigma_in->setRange(1.000, NUM_WORKER_THREADS-1);
		plq_sigma_in->addTerm(new Trapezoid("MODERATE", 1.000, 1.000, 2.000, 5.000));
		plq_sigma_in->addTerm(new Trapezoid("INTENSIVE", 2.000, 3.500, NUM_WORKER_THREADS-1, NUM_WORKER_THREADS-1));
		engine->addInputVariable(plq_sigma_in);
		wlq_rho_in = new InputVariable();
		wlq_rho_in->setEnabled(true);
		wlq_rho_in->setName("WLQ_RHO");
		wlq_rho_in->setRange(0.000, 2.000);
		wlq_rho_in->addTerm(new Trapezoid("FAST", 0.000, 0.000, 0.500, 0.900));
		wlq_rho_in->addTerm(new Triangle("ACCEPTABLE", 0.500, 0.900, 1.300));
		wlq_rho_in->addTerm(new Trapezoid("SLOW", 0.900, 1.300, 2.000, 2.000));
		engine->addInputVariable(wlq_rho_in);
		// initialization of the output linguistic variables
		plq_degree_var_out = new OutputVariable();
		plq_degree_var_out->setEnabled(true);
		plq_degree_var_out->setName("PLQ_DEGREE_VAR");
		plq_degree_var_out->setRange(0.000, 2.000);
		plq_degree_var_out->fuzzyOutput()->setAccumulation(new Maximum);
		plq_degree_var_out->setDefuzzifier(new WeightedAverage("Automatic"));
		plq_degree_var_out->setDefaultValue(1.000);
		plq_degree_var_out->setLockPreviousOutputValue(false);
		plq_degree_var_out->setLockOutputValueInRange(false);
		plq_degree_var_out->addTerm(new Constant("DECREMENT", 0.500));
		plq_degree_var_out->addTerm(new Constant("SLIGHT_DECREMENT", 0.750));
		plq_degree_var_out->addTerm(new Constant("UNCHANGED", 1.000));
		plq_degree_var_out->addTerm(new Constant("SLIGHT_INCREMENT", 1.250));
		plq_degree_var_out->addTerm(new Constant("INCREMENT", 1.500));
		engine->addOutputVariable(plq_degree_var_out);
		wlq_degree_var_out = new OutputVariable();
		wlq_degree_var_out->setEnabled(true);
		wlq_degree_var_out->setName("WLQ_DEGREE_VAR");
		wlq_degree_var_out->setRange(0.000, 2.000);
		wlq_degree_var_out->fuzzyOutput()->setAccumulation(new Maximum);
		wlq_degree_var_out->setDefuzzifier(new WeightedAverage("Automatic"));
		wlq_degree_var_out->setDefaultValue(1.000);
		wlq_degree_var_out->setLockPreviousOutputValue(false);
		wlq_degree_var_out->setLockOutputValueInRange(false);
		wlq_degree_var_out->addTerm(new Constant("DECREMENT", 0.500));
		wlq_degree_var_out->addTerm(new Constant("SLIGHT_DECREMENT", 0.750));
		wlq_degree_var_out->addTerm(new Constant("UNCHANGED", 1.000));
		wlq_degree_var_out->addTerm(new Constant("SLIGHT_INCREMENT", 1.250));
		wlq_degree_var_out->addTerm(new Constant("INCREMENT", 1.500));
		engine->addOutputVariable(wlq_degree_var_out);
		// definition of the fuzzy logic rules of the strategy
		ruleBlock = new RuleBlock();
		ruleBlock->setEnabled(true);
		ruleBlock->setName("Rule_Block");
		ruleBlock->setConjunction(new Minimum);
		ruleBlock->setDisjunction(new Maximum);
		ruleBlock->setActivation(new Minimum);
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is FAST and WLQ_RHO is FAST then PLQ_DEGREE_VAR is DECREMENT and WLQ_DEGREE_VAR is DECREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is FAST and WLQ_RHO is ACCEPTABLE then PLQ_DEGREE_VAR is DECREMENT and WLQ_DEGREE_VAR is UNCHANGED", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is FAST and WLQ_RHO is SLOW then PLQ_DEGREE_VAR is DECREMENT and WLQ_DEGREE_VAR is INCREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is MODERATE and WLQ_RHO is FAST then PLQ_DEGREE_VAR is UNCHANGED and WLQ_DEGREE_VAR is DECREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is MODERATE and WLQ_RHO is ACCEPTABLE then PLQ_DEGREE_VAR is UNCHANGED and WLQ_DEGREE_VAR is UNCHANGED", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is MODERATE and WLQ_RHO is SLOW then PLQ_DEGREE_VAR is UNCHANGED and WLQ_DEGREE_VAR is INCREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is INTENSIVE and WLQ_RHO is FAST then PLQ_DEGREE_VAR is SLIGHT_INCREMENT and WLQ_DEGREE_VAR is DECREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is INTENSIVE and WLQ_RHO is ACCEPTABLE then PLQ_DEGREE_VAR is SLIGHT_INCREMENT and WLQ_DEGREE_VAR is UNCHANGED", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is ACCEPTABLE and PLQ_SPLITTING is INTENSIVE and WLQ_RHO is SLOW then PLQ_DEGREE_VAR is UNCHANGED and WLQ_DEGREE_VAR is INCREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is SLOW and WLQ_RHO is FAST then PLQ_DEGREE_VAR is INCREMENT and WLQ_DEGREE_VAR is SLIGHT_DECREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is SLOW and WLQ_RHO is ACCEPTABLE then PLQ_DEGREE_VAR is INCREMENT and WLQ_DEGREE_VAR is SLIGHT_INCREMENT", engine));
		ruleBlock->addRule(fl::Rule::parse("if PLQ_RHO is SLOW and WLQ_RHO is SLOW then PLQ_DEGREE_VAR is SLIGHT_INCREMENT and WLQ_DEGREE_VAR is INCREMENT", engine));
		engine->addRuleBlock(ruleBlock);
		// engine configuration
		engine->configure("Minimum", "Maximum", "Minimum", "Maximum", "WeightedAverage");
		// ******************************************END INITIALIZATION OF THE FUZZY ADAPTATION STRATEGY****************************************** //
	}

	// destructor
	~FuzzyStrategy() {}

	/* 
	 * Method to evaluate the adaptation strategy. It returns the new parallelism degrees
	 * of the first and the second stage according to the evaluation of the FCS.
	 */
	pair<size_t, size_t> evaluate(double plq_rho_val, double plq_sigma_val, double wlq_rho_val, size_t nw_plq, size_t nw_wlq) {
		pair<size_t, size_t> new_nws;
		// evaluation of the fuzzy strategy
		plq_rho_in->setInputValue(min(2.0, plq_rho_val));
		plq_sigma_in->setInputValue(plq_sigma_val);
		wlq_rho_in->setInputValue(min(2.0, wlq_rho_val));
		engine->process();
		double plq_var = plq_degree_var_out->getOutputValue();
		double wlq_var = wlq_degree_var_out->getOutputValue();
		//cout << "plq_degree_var_out: " << plq_var << " wlq_degree_var_out: " << wlq_var << endl;
		//cout << "Fuzzy strategy evaluation: [PLQ pardegree old: " << nw_plq << ", new: " << nw_plq * plq_var << "] [WLQ pardegree old: " << nw_wlq << ", new: " << nw_wlq * wlq_var << "]" << endl;
		// prepare the final parallelism degrees
		new_nws.first = round(nw_plq * plq_var);
		new_nws.second = round(nw_wlq * wlq_var);
		// special cases of very small parallelism degrees
		if((nw_plq == 1) && (plq_var >= 1.250)) new_nws.first = 2;
		if((nw_plq == 2) && (plq_var >= 0.563 && plq_var < 1.250)) new_nws.first = 2;
		if((nw_plq == 3) && (plq_var >= 0.793 && plq_var < 1.17)) new_nws.first = 3;
		if((nw_wlq == 1) && (wlq_var >= 1.250)) new_nws.second = 2;
		if((nw_wlq == 2) && (wlq_var >= 0.563 && wlq_var < 1.250)) new_nws.second = 2;
		if((nw_wlq == 3) && (wlq_var >= 0.793 && wlq_var < 1.17)) new_nws.second = 3;
		if(new_nws.first < 1) new_nws.first = 1;
		if(new_nws.second < 1) new_nws.second = 1;
		// check whether there are sufficient resources
		if(new_nws.first + new_nws.second > NUM_WORKER_THREADS) {
			// heuristics for parallelism degree reduction
			double reduction_factor = (((double) NUM_WORKER_THREADS) / (new_nws.first + new_nws.second));
			new_nws.first = round(reduction_factor * new_nws.first);
			new_nws.second = round(reduction_factor * new_nws.second);
			if(new_nws.first + new_nws.second > NUM_WORKER_THREADS) new_nws.first = new_nws.first - 1;
		}
		return new_nws;
	}
};

#endif
