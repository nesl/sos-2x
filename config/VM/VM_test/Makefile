# -*-Makefile-*-
###############################################
# Makefile for Virtual Machine
###############################################
PROJ = vm_config
ROOTDIR = ../../..

VPATH += $(ROOTDIR)/modules/VM/coreVM
VPATH += $(ROOTDIR)/modules/VM/script_loader



DEFS += -DINCL_VM 


SRCS += script_loader.c
SRCS += dvm.c DVMScheduler.c DVMResourceManager.c DVMEventHandler.c
SRCS += DVMConcurrencyMngr.c DVMBasiclib.c DVMStacks.c DVMqueue.c

include ../../Makerules

