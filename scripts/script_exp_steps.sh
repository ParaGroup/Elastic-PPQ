#!/bin/bash
# 
# Program: Script bash to execute the experiments with various elasticity steps and pid sampling intervals
# Date:    April 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Datasets
DATASETS="Game1"

# Elasticity steps
STEPS="1 2.5 5 10 20"

# PID intervals
PIDS="1000"

# Change the working directory
W_DIR="/home/mencagli/Elastic_PF"
cd ${W_DIR}

echo "Script started..."

# skyline query
for dataset in $DATASETS; do
	for step in $STEPS; do
		for pid in $PIDS; do
			echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_2x.dat"
			./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_2x.dat > /dev/null &
			sleep 1
			echo "pf_skyline_aspid_step${step}_pid${pid} -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1"
			((./bin/pf_skyline_aspid_step${step}_pid${pid} -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/steps/skyline\_output\_${dataset}\_step${step}_pid${pid}.txt
			mv ./log/workers_plq.log ./results/steps/skyline\_workers\_plq\_\_${dataset}\_step${step}_pid${pid}.log
			mv ./log/workers_wlq.log ./results/steps/skyline\_workers\_wlq\_\_${dataset}\_step${step}_pid${pid}.log
			mv ./log/client.log ./results/steps/skyline\_client\_\_${dataset}\_step${step}_pid${pid}.log
			mv ./log/controller.log ./results/steps/skyline\_controller\_\_${dataset}\_step${step}_pid${pid}.log
		done
	done
done

echo "...end"