#!/bin/bash
# 
# Program: Script bash to execute the Python scripts to generate the synthetic datasets.
# Date:    April 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Indices of dispersion of the MAP datasets
INDEX="3000 4000"

# Hurst parameters of the Self-Similar datasets
HURST="0.7 0.9"

# Change the working directory
W_DIR="/home/mencagli/Datasets/Synthetic/tools"
cd ${W_DIR}

echo "Script started..."

# MAP dataset generation:
for index in $INDEX; do
	echo "python ./tuples\_generator.py -n 10000000 -d 8 -c 0 -r 30000 -i ${index} -f raw\_dataset\_10M\_30Kts\_c0\_i${index}.txt"
	python ./tuples\_generator.py -n 10000000 -d 8 -c 0 -r 30000 -i ${index} -f raw\_dataset\_10M\_30Kts\_c0\_i${index}.txt
	mv arrivals.txt arrivals\_map\_i${index}.txt
	python ./add\_disorder.py -d 200000 -i raw\_dataset\_10M\_30Kts\_c0\_i${index}.txt -o dataset\_10M\_30Kts\_c0\_i${index}\_d200ms.dat
done

# Self-Similar dataset generation:
for hurst in $HURST; do
	echo "python ./tuples\_generator\_pareto.py -n 10000000 -d 8 -c 0 -r 30000 -h ${hurst} -f raw\_dataset\_10M\_30Kts\_c0\_h${hurst}.txt"
	python ./tuples\_generator\_pareto.py -n 10000000 -d 8 -c 0 -r 30000 -h ${hurst} -f raw\_dataset\_10M\_30Kts\_c0\_h${hurst}.txt
	mv arrivals.txt arrivals\_pareto\_h${hurst}.txt
	python ./add\_disorder.py -d 200000 -i raw\_dataset\_10M\_30Kts\_c0\_h${hurst}.txt -o dataset\_10M\_30Kts\_c0\_h${hurst}\_d200ms.dat
done

echo "...end"