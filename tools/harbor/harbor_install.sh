#! /bin/bash
#*********************************************************
# File: sandboxinstall.sh
# Author: Ram Kumar {ram@ee.ucla.edu}
# Description: Install the tools used for sandboxing
#*********************************************************
####################################################################
echo "=============================="
echo "====={ Build AVR Sandbox }===="
echo "=============================="
cd ./app/avrsandbox
make clean && ! make && echo "AVR Sandbox build failed" && exit 1
cd ../..
echo "=============================="
echo "=========={ SUCCESS }========="
echo "=============================="
####################################################################
echo "=============================="
echo "====={ Build AVR DisAsm. }===="
echo "=============================="
cd ./app/avrdisasm
make clean && ! make && echo "AVR Disasm. build failed" && exit 1
cd ../..
echo "=============================="
echo "=========={ SUCCESS }========="
echo "=============================="
####################################################################