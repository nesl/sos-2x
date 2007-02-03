#!/bin/sh

VERS="2.0.0"
DATE="2007-02-03"

# Leave the desired layout uncommented.
LAYOUT=layout          # Tables based layout.

ASCIIDOC_HTML="asciidoc --unsafe --backend=xhtml11 --attribute icons --attribute iconsdir=./images/icons --attribute=badges --attribute=revision=$VERS  --attribute=date=$DATE"

$ASCIIDOC_HTML --conf-file=${LAYOUT}.conf --attribute=index-only index.txt

cd publications
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. publications.txt
cd ..

cd tutorial/timesync
$ASCIIDOC_HTML --conf-file=../../${LAYOUT}.conf --attribute=styledir=../.. timesync.txt
cd ../..

cd tutorial
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. index.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. blink.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. blink_avrora.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. blink_sim.txt
$ASCIIDOC_HTML --conf-file=../${LAYOUT}.conf --attribute=styledir=.. module_prog_guide.txt
cd ..

