#!/bin/bash

if [ -z "${SOSROOT}" ]; then
	echo "SOSROOT environment variable is not defined"
	echo "Please add SOSROOT to environment variable and point to the top level"
	echo "SOS directory"
	exit 1
fi

## Script for running the blank kernel
java avrora/Main \
    -simulation=sensor-network -sections=.data,.text,.sos_bls\
    -monitors=sleep,serial\
    -ports=1:0:8314 \
    -show-packets=true\
    -show-tree-packets=true \
    -random-seed=151079 -random-start=[0,100000000] \
    -report-seconds -seconds-precision=2 -seconds=1800.0 \
    -topology=avrora_input_topology -nodecount=10 -update-node-id=true \
    $SOSROOT/config/blank/blank.od
