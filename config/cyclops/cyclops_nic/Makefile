PROJ = cyclops_nic_app
ROOTDIR = ../../..

# DEFS += -DNO_SOS_UART
DEFS += -DNO_SOS_I2C
DEFS += -DUSE_UART1



# Define USE_SOS_I2C to a non-empty value if you
# want to enable the I2C drivers.
# This is required here because this compiles
# the cyclops NIC.
USE_SOS_I2C = 1

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



SRCS += cyclops_nic.c

include ../../Makerules


vpath loader.c $(ROOTDIR)/extensions/loader/
vpath cyclops_nic.c $(ROOTDIR)/modules/cyclops/cyclops_nic/

