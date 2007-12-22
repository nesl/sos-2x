# -*-Makefile-*- #

PROJ = new_sensor_dump
OBJS = sos_catch.o
ROOTDIR = ../..
SB = tmote_impact

DEFS += -DNEW_SENSING_API

ifeq ($(SB), tmote)
DEFS += -DTMOTE_SENSOR_BOARD
# Path to platform specific sensor types file
INCDIR += -I$(ROOTDIR)/platform/tmote/include/
endif

ifeq ($(SB), tmote_invent)
DEFS += -DTMOTE_INVENT_SENSOR_BOARD
# Path to platform specific sensor types file
INCDIR += -I$(ROOTDIR)/platform/tmote/include/
endif

ifeq ($(SB), tmote_impact)
DEFS += -DTMOTE_IMPACT_SENSOR_BOARD
# Path to platform specific sensor types file
INCDIR += -I$(ROOTDIR)/platform/tmote/include/
endif

CFLAGS += $(DEFS)

SRCS += $(PROJ).c

# Path to new sensing API include files
INCDIR += -I$(ROOTDIR)/kernel/include/new_sensing_api/

# Library for sqlite3:
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
	$(CC) $(CFLAGS) $(INCDIRS) $(LDFLAGS) $(LIBS) $< -o $@ 
