
PROJ = blank

ROOTDIR = ../../..

# Define this variable to include the ViRe framework.
USE_VIRE_FRAMEWORK = 1

DEFS += -D'UART_ADDR=0x8000'

DEFS += -DWIRING_TEST_STAGE_6
DEFS += -DPUT_ROUTING_TABLE_IN_RAM
DEFS += -DUSE_VIRE_TOKEN_MEM

SRCS += loader.c
SRCS += script_loader.c
SRCS += combine.c combine_3.c
SRCS += truncate_long.c
SRCS += source.c transmit.c truncate.c led_display.c

#################################################
# MEMORY PROTECTION OPTIONS
#################################################
ifeq ($(MODE),sfi)
SRCS += sos_mod_verify.c
VPATH += $(ROOTDIR)/tools/harbor/soslib
INCDIR += -I$(ROOTDIR)/tools/harbor/soslib
INCDIR += -I$(ROOTDIR)/tools/harbor/lib
endif

INCDIR += -I$(ROOTDIR)/extensions/loader/

include ../../Makerules

vpath loader.c $(ROOTDIR)/extensions/loader/
vpath source.c $(ROOTDIR)/modules/vire/wiring_test/
vpath combine.c $(ROOTDIR)/modules/vire/wiring_test/
vpath combine_3.c $(ROOTDIR)/modules/vire/wiring_test/
vpath transmit.c $(ROOTDIR)/modules/vire/wiring_test/
vpath truncate.c $(ROOTDIR)/modules/vire/wiring_test/
vpath truncate_long.c $(ROOTDIR)/modules/vire/wiring_test/
vpath led_display.c $(ROOTDIR)/modules/vire/wiring_test/
vpath script_loader.c $(ROOTDIR)/modules/vire/wiring_test/script_loader/

