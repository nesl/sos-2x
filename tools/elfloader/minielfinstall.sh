#! /bin/bash
#*************************************************************
# File: minielfinstall.sh
# Author: Ram Kumar {ram@ee.ucla.edu}
# Description: Install the tools required for Mini-ELF loader
#*************************************************************
if [ -z "${SOSROOT}" ]; then
	echo "SOSROOT environment variable is not defined"
	echo "Please add SOSROOT to environment variable and point to the top level"
	echo "SOS directory"
	exit 1
fi

SOSBIN=${SOSROOT}/tools/bin/
####################################################################
echo "=============================="
echo "====={ Build ELF Reader }====="
echo "=============================="
cd ./utils/elfread
make clean && ! make avr && echo "ELF Reader build failed" && exit 1
cp elfread.exe ${SOSBIN}
cd ../..
echo "=============================="
echo "=========={ SUCCESS }========="
echo "=============================="
####################################################################
echo "==================================="
echo "====={ Build Mini-ELF Reader }====="
echo "==================================="
cd ./utils/melfread
make clean && ! make avr && echo "Mini-ELF Reader build failed" && exit 1
cp melfread.exe ${SOSBIN}
cd ../..
echo "=============================="
echo "=========={ SUCCESS }========="
echo "=============================="
####################################################################
echo "==============================="
echo "====={ Build ELF-to-Mini }====="
echo "==============================="
cd ./utils/elftomini
make clean && ! make avr && echo "ElfToMini build failed" && exit 1
cp elftomini.exe ${SOSBIN}
cd ../..
echo "=============================="
echo "=========={ SUCCESS }========="
echo "=============================="
####################################################################
