#!/bin/bash

#echo "install a blank kernel"
cd $SOSROOT/config/blank
make clean
make micaz
make install PROG=mib510 PORT=/dev/ttyUSB0 ADDRESS=0 SOS_GROUP=101

echo "install the sensor driver"
cd $SOSROOT/modules/sensordrivers/mts310/accel
make clean
make micaz > /dev/null
sos_tool.exe --insmod=accel_sensor.mlf

sleep 5

echo "install the test driver"
cd $SOSROOT/modules/unit_test/modules/sensordrivers/mts310/accel
make clean
make micaz > /dev/null
sos_tool.exe --insmod=accel_test.mlf
