#!/bin/bash
# 
# Program: Script bash to execute the benchmark of the PLQ splitting.
# Date:    March 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Parallelism degrees of the PLQ
PAR_DEGREES="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20"

# Change the working directory
W_DIR="/home/mencagli/Elastic_PF"
cd ${W_DIR}

echo "Script started..."

# skyline query AS_PID: dataset MAP I=1K
for pardegree in $PAR_DEGREES; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_i1K_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_i1K_d200ms.dat > /dev/null &
	sleep 1
	echo "plq_skyline_aspid_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}"
	((./bin/plq_skyline_aspid_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}) < map.txt) > ./results/splitting/output\_aspid\_i1K\_degree${pardegree}.txt
	mv ./log/workers_plq.log ./results/splitting/workers\_plq\_aspid\_i1K\_degree${pardegree}.log
	mv ./log/client_plq.log ./results/splitting/client\_plq\_aspid\_i1K\_degree${pardegree}.log
	mv ./log/controller.log ./results/splitting/controller\_aspid\_i1K\_degree${pardegree}.log
done

# skyline query TB_RR: dataset MAP I=1K
for pardegree in $PAR_DEGREES; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_i1K_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_i1K_d200ms.dat > /dev/null &
	sleep 1
	echo "plq_skyline_tb_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}"
	./bin/plq_skyline_tb_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree} > ./results/splitting/output\_tb\_i1K\_degree${pardegree}.txt
	mv ./log/workers_plq.log ./results/splitting/workers\_plq\_tb\_i1K\_degree${pardegree}.log
	mv ./log/client_plq.log ./results/splitting/client\_plq\_tb\_i1K\_degree${pardegree}.log
	mv ./log/controller.log ./results/splitting/controller\_tb\_i1K\_degree${pardegree}.log
done

# skyline query AS_PID: dataset Pareto H=0.6
for pardegree in $PAR_DEGREES; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_h0.6_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_h0.6_d200ms.dat > /dev/null &
	sleep 1
	echo "plq_skyline_aspid_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}"
	((./bin/plq_skyline_aspid_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}) < pareto.txt) > ./results/splitting/output\_aspid\_h0.6\_degree${pardegree}.txt
	mv ./log/workers_plq.log ./results/splitting/workers\_plq\_aspid\_h0.6\_degree${pardegree}.log
	mv ./log/client_plq.log ./results/splitting/client\_plq\_aspid\_h0.6\_degree${pardegree}.log
	mv ./log/controller.log ./results/splitting/controller\_aspid\_h0.6\_degree${pardegree}.log
done

# skyline query TB_RR: dataset Pareto H=0.6
for pardegree in $PAR_DEGREES; do
	echo "./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_h0.6_d200ms.dat"
	./bin/generator -h localhost -p 10000 -f ${HOME}/Datasets/Synthetic/data/dataset_10M_80Kts_c0_h0.6_d200ms.dat > /dev/null &
	sleep 1
	echo "plq_skyline_tb_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree}"
	./bin/plq_skyline_tb_static -p 10000 -d 0 -w 1000 -s 100 -n ${pardegree} > ./results/splitting/output\_tb\_h0.6\_degree${pardegree}.txt
	mv ./log/workers_plq.log ./results/splitting/workers\_plq\_tb\_h0.6\_degree${pardegree}.log
	mv ./log/client_plq.log ./results/splitting/client\_plq\_tb\_h0.6\_degree${pardegree}.log
	mv ./log/controller.log ./results/splitting/controller\_tb\_h0.6\_degree${pardegree}.log
done

echo "...end"