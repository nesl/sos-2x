#!/bin/sh

VERS="2.0.0"
DATE="2007-02-03"

# Leave the desired layout uncommented.
LAYOUT=layout          # Tables based layout.

ASCIIDOC_HTML="asciidoc --unsafe --backend=xhtml11 --conf-file=${LAYOUT}.conf --attribute icons --attribute iconsdir=./images/icons --attribute=badges --attribute=revision=$VERS  --attribute=date=$DATE"

$ASCIIDOC_HTML --attribute=index-only index.txt
#$ASCIIDOC_HTML --attribute=numbered userguide.txt
#$ASCIIDOC_HTML --doctype=manpage manpage.txt
#$ASCIIDOC_HTML downloads.txt
#$ASCIIDOC_HTML README.txt
#$ASCIIDOC_HTML CHANGELOG.txt
#$ASCIIDOC_HTML README-website.txt
#$ASCIIDOC_HTML a2x.1.txt

