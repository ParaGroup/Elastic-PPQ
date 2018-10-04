# Elastic-PPQ
A Framework to support Elastic Parallel Preference Queries on Multicore Architectures

This framework is an extension of BT-PPQ. In addition to burst-tolerant scheduling strategies, the framework supports elasiticty mechanisms able to dynamically change the parallelism level of the system. The whole adaptation logic is programmed by a Fuzzy Logic Controller.

The code has been used in the paper "Elastic-PPQ: a Two-level Autonomic System for Spatial Preference Query Processing over Dynamic Data Streams". Future Generation Computer Systems, Volume 79, Part 3, 862-877, 2018.

The code requires FastFlow 2.1 (http://calvados.di.unipi.it/) and FuzzyLite 5 (http://www.fuzzylite.com/cpp/).
