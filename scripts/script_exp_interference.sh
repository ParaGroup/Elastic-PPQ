#!/bin/bash
# 
# Program: Script bash to execute the experiments with external interferences.
# Date:    April 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Interference levels
LEVELS="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0"

# Change the working directory
W_DIR="/home/mencagli/Elastic_PF"
cd ${W_DIR}

echo "Script started..."

# skyline query AS_PID (Game1 datasets)
for level in $LEVELS; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_Game1\_2x.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Soccer/data/dataset\_Game1\_2x.dat > /dev/null &
	sleep 1
	echo "/home/mencagli/Generic\_Tools/cpu\_burn -n 30 -i ${level}"
	/home/mencagli/Generic\_Tools/cpu\_burn -n 30 -i ${level} &
	sleep 1
	echo "pf_skyline_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1"
	((./bin/pf_skyline_aspid -p 10000 -d 0.05 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/interference/skyline\_output\_${level}.txt
	mv ./log/workers_plq.log ./results/interference/skyline\_workers\_plq\_${level}.log
	mv ./log/workers_wlq.log ./results/interference/skyline\_workers\_wlq\_${level}.log
	mv ./log/client.log ./results/interference/skyline\_client\_${level}.log
	mv ./log/controller.log ./results/interference/skyline\_controller\_${level}.log
	wait
done

# skyline query AS_PID (MAP I=1K datasets)
# for level in $LEVELS; do
# 	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_i1K\_d200ms.dat"
# 	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset\_10M\_30Kts\_c0\_i1K\_d200ms.dat > /dev/null &
# 	sleep 1
# 	echo "/home/mencagli/Generic\_Tools/cpu\_burn -n 30 -i ${level}"
# 	/home/mencagli/Generic\_Tools/cpu\_burn -n 30 -i ${level} &
# 	sleep 1
# 	echo "pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1"
# 	((./bin/pf_skyline_aspid -p 10000 -d 0 -w 1000 -s 200 -n 1 -m 1) < split.txt) > ./results/interference/skyline\_output\_${level}.txt
# 	mv ./log/workers_plq.log ./results/interference/skyline\_workers\_plq\_${level}.log
# 	mv ./log/workers_wlq.log ./results/interference/skyline\_workers\_wlq\_${level}.log
# 	mv ./log/client.log ./results/interference/skyline\_client\_${level}.log
# 	mv ./log/controller.log ./results/interference/skyline\_controller\_${level}.log
# 	wait
# done

echo "...end"