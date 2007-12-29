# -*-Makefile-*- #

PROJ = new_sensor_dump
OBJS = sos_catch.o
ROOTDIR = ../../..

# Set the necessary sensor board for correct sensor
# definitions.
# {tmote, tmote_impact, tmote_invent}
SB = tmote_impact

# Path for sos_catch system
VPATH += ../

# This system is only for gathering data using
# the new sensing API (currently released for tmote only)
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

include ../../sos_server/lib/Makesossrvlib

