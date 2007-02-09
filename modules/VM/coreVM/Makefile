# -*-Makefile-*-
#######################
# Makefile for DVM
# Builds a single module for the entire Virtual machine
# VM allows extension points
######################

PROJ = dvm
ROOTDIR = ../../..
VPATH += $(ROOTDIR)/modules/VM/coreVM
INCDIR += -I$(ROOTDIR)/modules/include/VM


SRCS += DVMScheduler.c DVMConcurrencyMngr.c DVMResourceManager.c DVMEventHandler.c
SRCS += DVMBasiclib.c DVMStacks.c DVMqueue.c

include ../../Makerules