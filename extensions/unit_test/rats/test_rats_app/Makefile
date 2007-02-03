
PROJ = test_rats_app
ROOTDIR = ../../../..
SRCS += rats.c
SRCS += linear.c
SRCS += ../test_rats/test_rats.c
include $(ROOTDIR)/config/Makerules

LDFLAGS += -lm

vpath rats.c $(ROOTDIR)/extensions/rats/
vpath linear.c $(ROOTDIR)/extensions/rats
vpath test_rats.c ../test_rats
