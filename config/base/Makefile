PROJ = base

ROOTDIR = ../..

DEFS += -D'UART_ADDR=0x8000'
#DEFS += -DSOS_DEBUG_SCHED
#DEFS += -DSOS_DEBUG_DFT_LOADER 
#DEFS += -DSOS_DEBUG_FETCHER

#Choose the appropriate sensor board
#DEFS += -DMICASB
#DEFS += -DMTS400SB

#SRCS += dft_loader.c 
SRCS += loader.c nic.c

# Include simulated module 
SIM_SRCS += blink.c surge.c tree_routing.c

include ../Makerules

vpath loader.c $(ROOTDIR)/modules/management/loader/
vpath nic.c $(ROOTDIR)/modules/management/nic/
vpath blink.c $(ROOTDIR)/modules/test_modules/blink/
vpath surge.c $(ROOTDIR)/modules/test_modules/surge/
vpath tree_routing.c $(ROOTDIR)/modules/routing/tree_routing/
