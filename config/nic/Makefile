PROJ = nic

ROOTDIR = ../..

DEFS += -D'UART_ADDR=0x8000'
#DEFS += -DSOS_DEBUG_SCHED
#DEFS += -DSOS_DEBUG_DFT_LOADER 
#DEFS += -DSOS_DEBUG_FETCHER

#Choose the appropriate sensor board

SRCS += nic.c

include ../Makerules

vpath nic.c $(ROOTDIR)/modules/management/nic/
