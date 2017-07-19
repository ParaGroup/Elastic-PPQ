#!/bin/bash
# 
# Program: Script bash to execute the C++ program to generate the real datasets.
# Date:    March 2017
# Author:  Gabriele Mencagli <mencagli@di.unipi.it>

# Name of the real datasets to generate
DATASETS="Game1 Game2 Game3 Game4"

# Acceleration ratio of the datasets
speed="2x"

# Change the working directory
W_DIR="/home/mencagli/Datasets/Soccer/tools"
cd ${W_DIR}

echo "Script started..."

# dataset generation:
for dataset in $DATASETS; do
	echo "./soccer -a 2 -n -1 -i ../data/unfiltered\_${dataset}.csv -o dataset\_${dataset}\_${speed}.dat"
	./soccer -a 2 -n -1 -i ../data/unfiltered\_${dataset}.csv -o dataset\_${dataset}\_${speed}.dat
	mv arrivals.txt arrivals\_${dataset}\_${speed}.txt
done

echo "...end"