
PROJ = vm_config
ROOTDIR = ../../..

DEFS += -DMICASB
VPATH += $(ROOTDIR)/modules/VM/coreVM
VPATH += $(ROOTDIR)/modules/VM/script_loader
VPATH += $(ROOTDIR)/extensions/loader

#INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/

DEFS += -DINCL_VM -DLINK_KERNEL -DSOS_NO_LATE_BINDING
#DEFS += -DSOS_DEBUG_DFT_LOADER

SRCS += loader.c 
#phototemp_sensor.c
SRCS += script_loader.c
SRCS += DVMConcurrencyMngr.c DVMStacks.c DVMEventHandler.c
SRCS += DVMqueue.c DVMBuffer.c DVMinit.c DVMMathlib.c
SRCS += DVMScheduler.c DVMBasiclib.c DVMResourceManager.c

include ../../Makerules

vpath blink.c $(ROOTDIR)/modules/test_modules/blink/
vpath phototemp_sensor.c  $(ROOTDIR)/modules/sensordrivers/mts310/photo_temp/
