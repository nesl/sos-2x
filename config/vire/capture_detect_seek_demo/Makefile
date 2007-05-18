PROJ = test_soswire
ROOTDIR = ../../..

# Define this variable to include the ViRe framework.
USE_VIRE_FRAMEWORK = 1

DEFS += -D'I2C_ADDR= 15'

DEFS += -DPUT_ROUTING_TABLE_IN_RAM
DEFS += -DUSE_VIRE_TOKEN_MEM

###################################################
# LOADER OPTIONS
###################################################
SRCS += loader.c
#ifeq ($(LOADER), melf)
#DEFS += -DMINIELF_LOADER
#SRCS += melfloader.c minielfendian.c
#VPATH += $(ROOTDIR)/tools/elfloader/minielf
#VPATH += $(ROOTDIR)/tools/elfloader/soslib
#INCDIR += -I$(ROOTDIR)/tools/elfloader/soslib
#INCDIR += -I$(ROOTDIR)/tools/elfloader/minielf
#endif

#################################################
# MEMORY PROTECTION OPTIONS
#################################################
ifeq ($(MODE),sfi)
SRCS += sos_mod_verify.c
VPATH += $(ROOTDIR)/tools/harbor/soslib
INCDIR += -I$(ROOTDIR)/tools/harbor/soslib
INCDIR += -I$(ROOTDIR)/tools/harbor/lib
endif

###################################################
# COMPILED IN MODULES
###################################################
SRCS += frame_grabber.c update_background.c 
SRCS += select_transmit.c average_background.c absolute_subtract.c 
SRCS += max_locate.c over_thresh.c send_image.c

INCDIR += -I$(ROOTDIR)/extensions/loader/

###################################################
# DEBUGGING
###################################################

include ../../Makerules


vpath loader.c $(ROOTDIR)/extensions/loader/
vpath frame_grabber.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/frame_grabber/
vpath update_background.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/update_background/
vpath average_background.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/average_background/
vpath absolute_subtract.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/subtract/
vpath max_locate.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/max_locate/
vpath over_thresh.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/over_thresh/
vpath select_transmit.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/select_transmit/
vpath send_image.c $(ROOTDIR)/modules/vire/cyclops/capture_detect_seek_demo/send/
