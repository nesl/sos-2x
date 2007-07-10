# -*-Makefile-*- #

PROJ = sos_catch
ROOTDIR = ../..

SRCS += $(PROJ).c

# The path to the tree routing header:
INCDIR += -I$(ROOTDIR)/modules/routing/tree_routing/

# The path to the surge header:
INCDIR += -I$(ROOTDIR)/modules/demos/surge/

# Library and search path for sqlite3:
LDFLAGS += -lsqlite3
# You'll probably have to edit this search path as this corresponds to a
# MacPorts install of sqlite3 on a PowerPC Mac. Your installation of sqlite
# might already be on the library search path, but if it's not, just
# do "locate sqlite.h" and change this flag (the entire part after the -L)
# to that directory.
LIBS += -L/opt/local/include

# Default rule so that typing just 'make' won't try to build setup_db.exe.
# you should use 'make <arch>' anyways, there's no need to change this.
default:
	@echo 'You must specify an architecture (for example, "make x86").'

# A rule for making setup_db.exe without using the normal SOS build system. This
# is necessary because setup_db.exe is not an SOS program, but is in the
# same directory as sos_catch.c because they are part of the same system.
setup_db.exe: setup_db.c
	$(CC) $(CFLAGS) $(INCDIRS) $(LDFLAGS) $(LIBS) setup_db.c -o setup_db.exe

# the default target is phony.
.PHONY: default

include ../sos_server/lib/Makesossrvlib
