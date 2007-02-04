#!/bin/bash
#$Id: build-mspgcc,v 1.9 2006/08/23 20:48:17 cssharp Exp $
#@author Cory Sharp <cssharp@eecs.berkeley.edu>
#
# Copied from TinyOS 1.x, " build-mspgcc,v 1.9 2006/08/23 20:48:17 cssharp Exp"

OPT="$1"
if [ x$OPT = x ]
then
  cat <<"EOF"
usage: build-mspgcc install
$Id: build-mspgcc,v 1.9 2006/08/23 20:48:17 cssharp Exp $

build-mspgcc downloads, extracts, builds, and installs binutils, gcc,
and libc for the TI MSP430 microcontroller.  As root, run:

  ./build-mspgcc install

The default installation directory is $SOSTOOLDIR.  Change this by
setting the environment variable INSTALL_DIR before compiling and
installing:

  INSTALL_DIR=/usr/local/msp430 ./build-mspgcc install

All other build options are unsupported.

Unsupported usage: build-mspgcc [command] (package)
commands: get, extract, build, install
packages: MSPGCC_CVS, BINUTILS, GCC, LIBC
environment variables:
  INSTALL_DIR
  USE_GCC=3.2 or 3.3
    - currently 3.3 does not include msp430 1611 support
EOF
  exit 0
fi

[ $OPT = get ] && OPT_NUM=1
[ $OPT = extract ] && OPT_NUM=2
[ $OPT = build ] && OPT_NUM=3
[ $OPT = install ] && OPT_NUM=4
[ x$OPT_NUM = x ] && echo "invalid option" && exit 1

PACKAGE="$2"

START_DIR=$PWD
: ${ARCHIVE_DIR:=archive}
: ${BUILD_DIR:=build}
: ${INSTALL_DIR:=$SOSTOOLDIR}

# {package}_URL will by default derive
#   {package}_ARCHIVE for the tarball, and
#   {package}_DIR for the source directory extracted from the tarball
# which can be overridden just by specifying the variable.

# Each package can specify following Bash functions 
#   {package}_get
#   {package}_extact
#   {package}_build
#   {package}_install
# which take on defaults defined below if unspecified.

if [ `uname -m` == i386 ]
then
	echo "Installing toolchain on an Intel Mac"
	echo "Installation directory is $INSTALL_DIR"
	echo "Make sure that $INSTALL_DIR/msp430/bin is added to the system PATH variable!"
	cd $INSTALL_DIR
	wget http://userfs.cec.wustl.edu/%7Ekak1/tinyos2_mac_install/msp430_tools-i686-apple-darwin.tar.gz
	tar -zxvf msp430_tools-i686-apple-darwin.tar.gz
	rm msp430_tools-i686-apple-darwin.tar.gz
	cd $START_DIR
else
BINUTILS_URL="ftp://ftp.gnu.org/gnu/binutils/binutils-2.17.tar.bz2"
GCC32_URL="ftp://ftp.gnu.org/gnu/gcc/gcc-3.2.3/gcc-core-3.2.3.tar.bz2"
GCC33_URL="ftp://ftp.gnu.org/gnu/gcc/gcc-3.3.6/gcc-core-3.3.6.tar.bz2"
GCC34_URL="http://ftp.gnu.org/gnu/gcc/gcc-3.4.6/gcc-core-3.4.6.tar.bz2"
MSPGCC_CVS_ARCHIVE=mspgcc-cvs.tar.gz
MSPGCC_CVS_DATE="1 Aug 2006"   # "now" is an option, see 1 Jun 2005 discussion thread "[tinyos-msp430] msp430 compiler tools"
#MSPGCC_CVS_DATE="now"   # "now" is an option, see 1 Jun 2005 discussion thread "[tinyos-msp430] msp430 compiler tools"
LIBC_ARCHIVE="$MSPGCC_CVS_ARCHIVE"
LIBC_DIR="mspgcc-cvs/msp430-libc/src"

# RB: HACKED TO USE GCC 3.3 for Intel Mac instead of GCC 3.2
if [ `uname -m` == i386 ]
then
	GCC_URL="$GCC33_URL"
else
	GCC_URL="$GCC32_URL"
	[ x$USE_GCC = x3.3 ] && GCC_URL="$GCC33_URL"
fi


### --- binutils

BINUTILS_build() {
  perl -i.orig -pe 's/define (LEX_DOLLAR) 0/undef $1/' gas/config/tc-msp430.h || exit 1
  ./configure --target=msp430 --prefix=$INSTALL_DIR || exit 1
  make || exit 1
}

### --- gcc

GCC_build() {
  builtin pushd $START_DIR || exit 1
  builtin pushd $BUILD_DIR || exit 1
  BUILD_BASE=$PWD
  builtin popd
  builtin popd
  if [ `uname -m` == i386 ]
  then
  	cp -R -L -p "$BUILD_BASE"/mspgcc-cvs/gcc/gcc-3.4/* . || exit 1
  else
  	if [ x$USE_GCC = x3.3 ]
  	then
  	  cp -r "$BUILD_BASE"/mspgcc-cvs/gcc/gcc-3.4/* . || exit 1
  	else
  	  cp -r "$BUILD_BASE"/mspgcc-cvs/gcc/gcc-3.3/* . || exit 1
  	fi
  fi
  GCC_SRCDIR="$PWD"
  GCC_OBJDIR="$PWD-obj"
  [ -d "$GCC_OBJDIR" ] || mkdir -p "$GCC_OBJDIR"
  cd "$GCC_OBJDIR"
  "$GCC_SRCDIR"/configure --target=msp430 --prefix="$INSTALL_DIR" || exit 1
  make || exit 1
  cd "$GCC_SRCDIR"
}

GCC_install() {
  GCC_SRCDIR="$PWD"
  GCC_OBJDIR="$PWD-obj"
  cd "$GCC_OBJDIR"
  make install || exit 1
  cd "$GCC_SRCDIR"
}

### --- mspgcc cvs

MSPGCC_CVS_get() {
  cd $ARCHIVE_DIR
  mkdir -p mspgcc-cvs
  cd mspgcc-cvs
  SF_CVS_SERVER=":pserver:anonymous@mspgcc.cvs.sourceforge.net"
  if [ -f $HOME/.cvspass ] && grep -q -F "$SF_CVS_SERVER:" $HOME/.cvspass
  then
    :
  else
    echo
    echo "***  Logging into SourceForge for anonymous CVS access."
    echo "***  Press ENTER when prompted for a password."
    echo
    cvs -d$SF_CVS_SERVER:/cvsroot/mspgcc login
  fi
  cvs -z3 -d$SF_CVS_SERVER:/cvsroot/mspgcc export -D "$MSPGCC_CVS_DATE" gcc msp430-libc
  patch -p1 <<EOF
diff -N -r -u mspgcc-cvs/gcc/gcc-3.3/gcc/config/msp430/msp430.c mspgcc-cvs-moteiv/gcc/gcc-3.3/gcc/config/msp430/msp430.c
--- mspgcc-cvs/gcc/gcc-3.3/gcc/config/msp430/msp430.c   2006-04-23 09:18:20.000000000 -0700
+++ mspgcc-cvs-moteiv/gcc/gcc-3.3/gcc/config/msp430/msp430.c    2006-08-02 13:47:05.953125000 -0700
@@ -6157,7 +6157,7 @@
       rtx pat = PATTERN (next);
       rtx src, t;

-      if (GET_CODE (pat) == RETURN)
+      if ((GET_CODE (pat) == RETURN) || (GET_CODE (pat) == PARALLEL))
 	return UNKNOWN;

       src = SET_SRC (pat);
EOF
  cd $START_DIR
  cd $ARCHIVE_DIR
  tar cfvz $MSPGCC_CVS_ARCHIVE mspgcc-cvs
  rm -rf mspgcc-cvs
  cd $START_DIR
}
MSPGCC_CVS_build() {
  true
}
MSPGCC_CVS_install() {
  true
}

### --- libc cvs

LIBC_get() {
  MSPGCC_CVS_get
}

LIBC_build() {
  [ -d msp1 ] || mkdir msp1
  [ -d msp2 ] || mkdir msp2
  perl -i.orig -pe 's{^(prefix\s*=\s*)(.*)}{${1}'"$INSTALL_DIR"'}' Makefile
  make || exit 1
}

### --- defaults for get, extract, build, install

default_get() {
  get_url "$1" "$2" || exit 1
}

default_extract() {
  extract_tarx "$1" "$2" || exit 1
}

default_build() {
  make || exit 1
}

default_install() {
  make install || exit 1
}

### ---
### ---
### ---

is_dir_in_path() {
  perl -e '@p=split /:/, $ENV{PATH}; @g=grep { $_ eq "'"$1"'" } @p; exit (@g?0:1);'
}

get_url() {
  wget --passive-ftp "$1" -O "$2"
}

tar_compress_opt() {
  [ ${1##*gz}x == x ] && echo z && return
  [ ${1##*bz2}x == x ] && echo j && return
}

tar_topdir() {
  tar -tv`tar_compress_opt $1`f "$1" 2>/dev/null \
    | perl -e 'print $1 if <> =~ /^(?:\S+\s+){5}([^\/\n]+)/'
}

extract_tarx() {
  [ -d "$2" ] || mkdir -p "$2"
  tar -xv`tar_compress_opt $1`f "$1" -C "$2"
}

get_pkg_function() {
  type "${1}_$2" >/dev/null 2>/dev/null && echo "${1}_$2" && return
  echo "default_$2"
}

process_package() {
  PKG="$1"

  echo
  echo "processing $PKG"

  ###  get
  if [ $OPT_NUM -ge 1 ]
  then
    eval PKG_URL="\$${PKG}_URL"
    eval PKG_ARCHIVE="\$${PKG}_ARCHIVE"
    [ x"$PKG_ARCHIVE" = x ] && PKG_ARCHIVE="${PKG_URL##*/}"
    [ -d "$ARCHIVE_DIR" ] || mkdir -p "$ARCHIVE_DIR" || exit 1
    WORK_FILE="$ARCHIVE_DIR/$PKG_ARCHIVE"
    if [ -f "$WORK_FILE" ]
    then
      echo "download $PKG: found $WORK_FILE, skipping download"
    else
      echo "download $PKG: downloading $WORK_FILE"
      eval `get_pkg_function $PKG get` "$PKG_URL" "$WORK_FILE"
    fi
  fi

  ###  extract
  if [ $OPT_NUM -ge 2 ]
  then
    eval PKG_DIR="\$${PKG}_DIR"
    [ x"$PKG_DIR" = x ] && PKG_DIR=`tar_topdir "$WORK_FILE"`
    [ -d "$BUILD_DIR" ] || mkdir -p "$BUILD_DIR" || exit 1
    WORK_DIR="$BUILD_DIR/$PKG_DIR"
    if [ -d "$WORK_DIR" ]
    then
      echo "found $WORK_DIR, skipping decompress"
    else
      echo "decompressing $WORK_FILE to $WORK_DIR"
      eval `get_pkg_function $PKG extract` "$WORK_FILE" "$BUILD_DIR"
    fi
  fi

  ###  build
  if [ $OPT_NUM -ge 3 ]
  then
    BUILD_COMPLETE="$WORK_DIR/build-complete"
    if [ -f "$BUILD_COMPLETE" ]
    then
      echo "found $BUILD_COMPLETE, skipping build"
    else
      echo "building $PKG"
      cd "$WORK_DIR"
      eval `get_pkg_function $PKG build`
      cd "$START_DIR"
      touch "$BUILD_COMPLETE"
    fi
  fi

  ###  install
  if [ $OPT_NUM -ge 4 ]
  then
    echo "installing $PKG"
    cd "$WORK_DIR"
    eval `get_pkg_function $PKG install`
    cd "$START_DIR"
  fi
}

### ---

# Force path to something explicit to avoid seriously weird errors
export PATH="$INSTALL_DIR/bin:$PATH"
if [ x$PACKAGE = x ]
then
  process_package MSPGCC_CVS
  process_package BINUTILS
  process_package GCC
  process_package LIBC
else
  process_package $PACKAGE
fi

#The machine was either a PowerPC Mac or Unix
fi
