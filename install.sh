#!/bin/sh

HOST=`uname -s`
SOSROOT=`pwd`
SOSSRV_TARGET=x86
SOSBIN=${SOSROOT}/tools/bin/
SOSMODDIR=${SOSROOT}/tools/modules/
	
if [ "${HOST}" = "Darwin" ]; then
	echo "Host is Darwin"
	SOSSRV_TARGET=ppc;
else
	echo "Assume host to be x86"
fi;

echo "SOSROOT = ${SOSROOT}"

if [ ! -d ${SOSBIN} ]; then
mkdir ${SOSBIN};
fi;

echo ""
echo "=================================================================="
echo "===================== { Build SOS Server } ======================="
echo "=================================================================="
echo ""

cd ${SOSROOT}/tools/sos_server/bin
make clean && ! make ${SOSSRV_TARGET} && echo "Unable to compile sossrv" && exit 1
cp sossrv.exe ${SOSBIN}
cd ${SOSROOT}


echo ""
echo "=================================================================="
echo "================== { Build SOS Module tool } ====================="
echo "=================================================================="
echo ""

cd ${SOSROOT}/config/sos_tool/
make clean && ! make emu && echo "Unable to compile sos_tool" && exit 1
cp sos_tool.exe ${SOSBIN}/

cd ${SOSROOT}

echo ""
echo "=================================================================="
echo "============= { Build Jump Table Generator tool } ================"
echo "=================================================================="
echo ""

cd ${SOSROOT}/tools/sos_jumpgen/
make clean && ! make && echo "Unable to compile sos_jumpgen" && exit 1
cp sos_jumpgen.exe ${SOSBIN}/

echo ""
echo "=================================================================="
echo "============== { Build blank SOS for simulation} ================="
echo "=================================================================="
echo ""

cd ${SOSROOT}/config/blank/
make clean && ! make sim && echo "Unable to compile blank" && exit 1
cp blank.exe ${SOSBIN}
make clean
cd ${SOSROOT}

echo ""
echo "=================================================================="
echo "======================= { Build modules } = ======================"
echo "=================================================================="
echo ""
if [ ! -d ${SOSMODDIR} ]; then
mkdir ${SOSMODDIR};
fi;

cd ${SOSROOT}/modules/test_modules/blink/
make clean && ! make mica2 && echo "unable to compile blink" && exit 1
cp blink.sos ${SOSMODDIR}
make clean
cd ${SOSROOT}

cd ${SOSROOT}/modules/test_modules/fnclient/
make clean && ! make mica2 && echo "unable to compile function client" && exit 1
cp fnclient.sos ${SOSMODDIR}
make clean
cd ${SOSROOT}

cd ${SOSROOT}/modules/test_modules/fntestmod/
make clean && ! make mica2 && echo "unable to compile function server" && exit 1
cp fntestmod.sos ${SOSMODDIR}
make clean
cd ${SOSROOT}

cd ${SOSROOT}/modules/routing/tree_routing/
make clean && ! make mica2 && echo "unable to compile tree routing" && exit 1
cp tree_routing.sos ${SOSMODDIR}
make clean
cd ${SOSROOT}

cd ${SOSROOT}/modules/test_modules/surge/
make clean && ! make mica2 && echo "unable to compile tree routing" && exit 1
cp surge.sos ${SOSMODDIR}
make clean
cd ${SOSROOT}

echo ""
echo "==========================================="
echo "======== { Installation Complete } ========"
echo "==========================================="
echo ""

echo "IMPORTENT: PLEASE READ!"
echo ""

echo "Please add the following to your shell environment"
echo "SOSROOT=${SOSROOT}"
echo ""

echo "Please add the following to your PATH" 
echo "${SOSBIN}"
echo ""

echo "Modules are precompiled to mica2 target"
echo "They can be found in ${SOSMODDIR}"
echo ""

echo "[ Optional ]"
echo "execute default_test.sh to see module insertion of blink on 3 nodes"
echo "Note that this is module insertion in simulation"
echo "To simulate your own module,"
echo "you will need to add the header in config/blank/blank.c"
echo ""
echo ""


