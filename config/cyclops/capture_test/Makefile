PROJ = capture_test_app
ROOTDIR = ../../..

ifeq ($(I2C_ADDRESS),)
DEFS += -D'I2C_ADDR= 15'
endif

###################################################
# LOADER OPTIONS
###################################################
SRCS += loader.c
ifeq ($(LOADER), melf)
DEFS += -DMINIELF_LOADER
SRCS += melfloader.c minielfendian.c
VPATH += $(ROOTDIR)/tools/elfloader/minielf
VPATH += $(ROOTDIR)/tools/elfloader/soslib
INCDIR += -I$(ROOTDIR)/tools/elfloader/soslib
INCDIR += -I$(ROOTDIR)/tools/elfloader/minielf
endif

SRCS += capture_test.c
#DEFS += -DUSE_SERIAL_DUMP

include ../../Makerules

vpath loader.c $(ROOTDIR)/extensions/loader/
vpath capture_test.c $(ROOTDIR)/modules/cyclops/capture_test/
