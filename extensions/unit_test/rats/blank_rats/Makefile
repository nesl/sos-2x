
PROJ = blank_rats
ROOTDIR = ../../../..
SRCS += rats.c
SRCS += linear.c
SRCS += loader.c
include $(ROOTDIR)/config/Makerules

LDFLAGS += -lm

vpath loader.c $(ROOTDIR)/extensions/loader/
vpath rats.c $(ROOTDIR)/extensions/rats/
vpath linear.c $(ROOTDIR)/extensions/rats
