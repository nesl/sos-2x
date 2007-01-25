#!/bin/bash

if [ -z "${SOS_DIR}" ]; then
	echo "SOS_DIR environment variable is not defined"
	echo "Please add SOS_DIR to environment variable and point to the top level"
	echo "SOS directory"
	exit 1
fi

## Script for running the blank kernel
java avrora/Main \
    -simulation=sensor-network -sections=.data,.text,.sos_bls\
    -monitors=sos.monitor.SOSPacketMonitor,sleep,serial\
    -ports=1:0:8314 \
    -show-packets=true\
    -show-tree-packets=true \
    -random-seed=151079 -random-start=[0,100000000] \
    -report-seconds -seconds-precision=2 -seconds=1800.0 \
    -topology=avrora_input_topology -nodecount=10 -update-node-id=true \
    $SOS_DIR/config/blank/blank.od
