#!/bin/sh

VERS="2.0.1"
DATE="2007-06-29"

# Leave the desired layout uncommented.
LAYOUT=layout          # Tables based layout.

ASCIIDOC_HTML="asciidoc --unsafe --backend=xhtml11 --attribute icons --attribute iconsdir=./images/icons --attribute=badges --attribute=revision=$VERS  --attribute=date=$DATE"

$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only index.txt
$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only downloads.txt
$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only acknowledgments.txt
$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only policies.txt
$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only sosage.txt
$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute iconsdir=./icons --attribute=index-only todo.txt

cd publications
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. publications.txt
cd ..

cd tutorial/timesync
$ASCIIDOC_HTML --conf-file=../../${LAYOUT}.conf --attribute=styledir=../.. --attribute iconsdir=../../icons timesync.txt
cd ../..

cd tutorial
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. index.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. add_syscall.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. installation.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. blink.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. blink_avrora.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. blink_sim.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. module_prog_guide.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. surge_avrora.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. surge_sim.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. blink_mica2.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. surge_mica2.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. surge.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. debug.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. static_kernel.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. dvm.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. pysos.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. test_suite.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute iconsdir=../icons --attribute=styledir=.. generic_test.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute icondsir=../icons --attribute=styledir=.. generic_kernel_test.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute icondsir=../icons --attribute=styledir=.. ceiling_array.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute icondsir=../icons --attribute=styledir=.. sos_db.txt 
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute icondsir=../icons --attribute=styledir=.. sos_db_interpreter.txt 
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute icondsir=../icons --attribute=styledir=.. sos_db_routing.txt 

cd ..

cd api
doxygen doxyfile
cd ..

