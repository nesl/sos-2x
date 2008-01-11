#!/bin/bash

PREFIX=$SOSTOOLDIR
PATH=$PREFIX/bin:$PATH

echo PREFIX=$PREFIX
echo PATH=$PATH

if [ ! -z "`echo \`uname\` | grep Darwin`" ]
then
	if [ ! -z "`echo \`uname -m\` | grep Power`" ]
	then
		INSTALL_PLATFORM=PowerPCMac
		UISP_DIR=macosx
	else
		INSTALL_PLATFORM=IntelMac
		UISP_DIR=macosx_intel
	fi
else
	if [ ! -z "`echo \`uname\` | grep CYGWIN`" ]
	then
		INSTALL_PLATFORM=Cygwin
		UISP_DIR=winxp
	else
		INSTALL_PLATFORM=Unix
		UISP_DIR=linux
	fi
fi

#get the files
wget ftp://ftp.gnu.org/gnu/binutils/binutils-2.15.tar.gz
tar xvfz binutils-2.15.tar.gz
wget ftp://ftp.gnu.org/gnu/gcc/gcc-3.4.3/gcc-3.4.3.tar.bz2
tar xvfj gcc-3.4.3.tar.bz2
wget http://download.savannah.gnu.org/releases/avr-libc/avr-libc-1.4.5.tar.bz2
tar xvfj avr-libc-1.4.5.tar.bz2

# Build binutils
cd binutils-2.15
mkdir obj-avr
cd obj-avr
../configure --prefix=$PREFIX --target=avr --disable-nls
make
make install
cd ../..

# Build GCC
cd gcc-3.4.3
mkdir obj-avr
cd obj-avr
../configure --prefix=$PREFIX --target=avr --enable-languages=c --disable-nls --disable-libssp
if [[ ($INSTALL_PLATFORM == IntelMac) || ($INSTALL_PLATFORM == PowerPCMac) ]]
then 
	make CC="cc -no-cpp-precomp"
else
	make
fi
make install
cd ../..

# Build AVR LibC
cd avr-libc-1.4.5
./configure --prefix=$PREFIX --build=`./config.guess` --host=avr
make
make install
cd ..

# Get UISP for the mib510
#BIN_DIR=$SOSROOT/doc/executables
#mv $BIN_DIR/uisp/$UISP_DIR/uisp $SOSTOOLDIR/bin/
#chmod a+x $SOSTOOLDIR/bin/uisp

