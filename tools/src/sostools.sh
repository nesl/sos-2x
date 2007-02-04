PREFIX=$SOSTOOLDIR
PATH=$PREFIX/bin:$PATH

echo PREFIX=$PREFIX
echo PATH=$PATH

# get the files
wget http://www.mr511.de/software/libelf-0.8.6.tar.gz
tar xvfz libelf-0.8.6.tar.gz

# build elfloader
cd $SOSROOT/tools/elfloader/
./minielfinstall.sh
cp $SOSROOT/tools/elfloader/utils/elftomini/elftomini.exe $SOSTOOLDIR/bin

# build sos server
cd $SOSROOT/tools/sos_server/bin
if [ `uname` == Darwin ]
then
	make ppc
else
	make x86
fi
cp sossrv.exe $SOSTOOLDIR/bin

# build sos tool
cd $SOSROOT/config/sos_tool/
make emu
cp sos_tool.exe $SOSTOOLDIR/bin

cd $SOSROOT/tools/src
