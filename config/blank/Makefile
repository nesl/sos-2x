
PROJ = blank

ROOTDIR = ../..

DEFS += -D'UART_ADDR=0x8000'
#DEFS += -DSOS_TREE_ROUTING_DEBUG
#DEFS += -DSOS_DEBUG_CODEMEM
#DEFS += -DSOS_DEBUG_SCHED
#DEFS += -DSOS_DEBUG_MALLOC
#DEFS += -DSOS_DEBUG_DFT_LOADER 
#DEFS += -DSOS_DEBUG_FETCHER
#DEFS += -DSOS_DEBUG_SIM_INTERFACE
#Choose the appropriate sensor board
#DEFS += -DMICASB
#DEFS += -DMTS400SB
#DEFS += -DUSE_UART1
#DEFS += -DSOS_USE_LOGGING

SRCS += loader.c

#################################################
# MEMORY PROTECTION OPTIONS
#################################################
ifeq ($(MODE),sfi)
SRCS += sos_mod_verify.c
VPATH += $(ROOTDIR)/tools/sos_sandbox/soslib
INCDIR += -I$(ROOTDIR)/tools/sos_sandbox/soslib
INCDIR += -I$(ROOTDIR)/tools/sos_sandbox/lib
endif

# Include simulated module 
SIM_SRCS += blink.c surge.c tree_routing.c test_param.c
INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/
include ../Makerules


vpath loader.c $(ROOTDIR)/modules/management/loader/
vpath blink.c $(ROOTDIR)/modules/test_modules/blink/
vpath surge.c $(ROOTDIR)/modules/test_modules/surge/
vpath test_param.c $(ROOTDIR)/modules/test_modules/test_param/
vpath tree_routing.c $(ROOTDIR)/modules/routing/tree_routing/


