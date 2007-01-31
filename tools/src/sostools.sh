PREFIX=$SOSTOOLDIR
PATH=$PREFIX/bin:$PATH

echo PREFIX=$PREFIX
echo PATH=$PATH

# get the files
wget http://www.mr511.de/software/libelf-0.8.6.tar.gz
tar xvfz libelf-0.8.6.tar.gz

# Build libelf
cd libelf-0.8.6
patch -p 0 < ../libelf-0.8.6.patch
./configure --prefix=$PREFIX
make
make install
echo "#include <libelf/libelf.h>" > $PREFIX/include/libelf.h
cd ..

# build elfloader
cd $SOS_DIR/tools/elfloader/
./minielfinstall.sh
cp $SOS_DIR/tools/elfloader/utils/elftomini/elftomini.exe $SOSTOOLDIR/bin

# build sos server
cd $SOS_DIR/tools/sos_server/bin
make x86
cp sossrv.exe $SOSTOOLDIR/bin

# build sos tool
cd $SOS_DIR/config/sos_tool/
make emu
cp sos_tool.exe $SOSTOOLDIR/bin

cd $SOS_DIR/tools/src
