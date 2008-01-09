#!/bin/bash

PREFIX=$SOSTOOLDIR
PATH=$PREFIX/bin:$PATH

echo PREFIX=$PREFIX
echo PATH=$PATH

# get the files
wget http://www.mr511.de/software/libelf-0.8.6.tar.gz
tar xvfz libelf-0.8.6.tar.gz
cd libelf-0.8.6
./configure --prefix=$PREFIX
make
make install

# build elfloader
cd $SOSROOT/tools/elfloader/
./minielfinstall.sh
cp $SOSROOT/tools/elfloader/utils/elftomini/elftomini.exe $SOSTOOLDIR/bin

# build sos server
cd $SOSROOT/tools/sos_server/bin
# test returns 0 when true (if length of string
# is non-zero), which is opposite for 'if' statement
if test -z `echo \`uname\` | grep Darwin`
then
	make x86
else
	make ppc
fi
cp sossrv.exe $SOSTOOLDIR/bin

# build sos tool
cd $SOSROOT/tools/sos_tool/
make emu
cp sos_tool.exe $SOSTOOLDIR/bin

cd $SOSROOT/tools/src
