
PROJ = vm_config
ROOTDIR = ../../..

DEFS += -DMICASB
VPATH += $(ROOTDIR)/modules/VM/coreVM
VPATH += $(ROOTDIR)/modules/VM/script_loader
VPATH += $(ROOTDIR)/extensions/loader

INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/

DEFS += -DINCL_VM 
#-DLINK_KERNEL -DSOS_NO_LATE_BINDING
#DEFS += -DSOS_DEBUG_DFT_LOADER

#SRCS += loader.c 

SRCS += phototemp_sensor.c

SRCS += script_loader.c
SRCS += dvm.c DVMScheduler.c DVMResourceManager.c DVMEventHandler.c
SRCS += DVMConcurrencyMngr.c DVMBasiclib.c DVMStacks.c DVMqueue.c

include ../../Makerules

