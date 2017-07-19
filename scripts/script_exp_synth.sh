#!/bin/bash
# 
# Program: Script bash to execute the complete experiments with synthetic datasets.
# Date:    April 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Indices of dispersion of the MAP datasets
MAP="i3K i4K"

# Hurst parameters of the Self-Similar datasets
PARETO="h0.7 h0.9"

# Change the working directory
W_DIR="/home/mencagli/Elastic_PF"
cd ${W_DIR}

echo "Script started..."

# skyline query AS_PID (MAP datasets)
for map in $MAP; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${map}_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${map}_d200ms.dat > /dev/null &
	sleep 1
	echo "pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/synthetic/skyline\_output\_${map}.txt
	mv ./log/workers_plq.log ./results/synthetic/skyline\_workers\_plq\_${map}.log
	mv ./log/workers_wlq.log ./results/synthetic/skyline\_workers\_wlq\_${map}.log
	mv ./log/client.log ./results/synthetic/skyline\_client\_${map}.log
	mv ./log/controller.log ./results/synthetic/skyline\_controller\_${map}.log
done

# skyline query AS_PID (Pareto datasets)
for pareto in $PARETO; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${pareto}_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${pareto}_d200ms.dat > /dev/null &
	sleep 1
	echo "pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/synthetic/skyline\_output\_${pareto}.txt
	mv ./log/workers_plq.log ./results/synthetic/skyline\_workers\_plq\_${pareto}.log
	mv ./log/workers_wlq.log ./results/synthetic/skyline\_workers\_wlq\_${pareto}.log
	mv ./log/client.log ./results/synthetic/skyline\_client\_${pareto}.log
	mv ./log/controller.log ./results/synthetic/skyline\_controller\_${pareto}.log
done

# topd query AS_PID (MAP datasets)
for map in $MAP; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${map}_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${map}_d200ms.dat > /dev/null &
	sleep 1
	echo "pf_topd_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_topd_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/synthetic/topd\_output\_${map}.txt
	mv ./log/workers_plq.log ./results/synthetic/topd\_workers\_plq\_${map}.log
	mv ./log/workers_wlq.log ./results/synthetic/topd\_workers\_wlq\_${map}.log
	mv ./log/client.log ./results/synthetic/topd\_client\_${map}.log
	mv ./log/controller.log ./results/synthetic/topd\_controller\_${map}.log
done

# topd query AS_PID (Pareto datasets)
for pareto in $PARETO; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${pareto}_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_${pareto}_d200ms.dat > /dev/null &
	sleep 1
	echo "pf_topd_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_topd_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/synthetic/topd\_output\_${pareto}.txt
	mv ./log/workers_plq.log ./results/synthetic/topd\_workers\_plq\_${pareto}.log
	mv ./log/workers_wlq.log ./results/synthetic/topd\_workers\_wlq\_${pareto}.log
	mv ./log/client.log ./results/synthetic/topd\_client\_${pareto}.log
	mv ./log/controller.log ./results/synthetic/topd\_controller\_${pareto}.log
done

echo "...end"