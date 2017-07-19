#!/bin/bash
# 
# Program: Script bash to execute the complete experiments with real datasets.
# Date:    March 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Name of the real datasets to execute
DATASETS="Game1 Game2 Game3 Game4"

# Acceleration ratio of the datasets
speed="2x"

# Change the working directory
W_DIR="/home/mencagli/Elastic_PF"
cd ${W_DIR}

echo "Script started..."

# skyline query AS_PID:
for dataset in $DATASETS; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_${speed}.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_${speed}.dat > /dev/null &
	sleep 1
	echo "pf_skyline_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_skyline_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/real/skyline\_output\_${dataset}.txt
	mv ./log/workers_plq.log ./results/real/skyline\_workers\_plq\_${dataset}.log
	mv ./log/workers_wlq.log ./results/real/skyline\_workers\_wlq\_${dataset}.log
	mv ./log/client.log ./results/real/skyline\_client\_${dataset}.log
	mv ./log/controller.log ./results/real/skyline\_controller\_${dataset}.log
done

# topd query AS_PID:
for dataset in $DATASETS; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_${speed}.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_${dataset}\_${speed}.dat > /dev/null &
	sleep 1
	echo "pf_topd_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_topd_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/real/topd\_output\_${dataset}.txt
	mv ./log/workers_plq.log ./results/real/topd\_workers\_plq\_${dataset}.log
	mv ./log/workers_wlq.log ./results/real/topd\_workers\_wlq\_${dataset}.log
	mv ./log/client.log ./results/real/topd\_client\_${dataset}.log
	mv ./log/controller.log ./results/real/topd\_controller\_${dataset}.log
done

echo "...end"