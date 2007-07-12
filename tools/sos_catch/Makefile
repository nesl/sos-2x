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

# If the sqlite3 library isn't in the right place on your system, you'll need to
# add a -L option to LIBS here such as:
#LIBS += -L/opt/local/lib
# If sqlite3 were in /opt/local/lib.

include ../sos_server/lib/Makesossrvlib

# A rule for making setup_db.exe without using the normal SOS build system. This
# is necessary because setup_db.exe is not an SOS program, but is in the
# same directory as sos_catch.c because they are part of the same system.
setup_db.exe: setup_db.c
	$(CC) $(CFLAGS) $(INCDIRS) $(LDFLAGS) $(LIBS) setup_db.c -o setup_db.exe
