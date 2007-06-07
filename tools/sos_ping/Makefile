
PROJ = sos_ping
ROOTDIR = ../..

ADDRESS = 0x8000

#DEFS += -DSOS_DEBUG_CODEMEM
#DEFS += -DSOS_DEBUG_FETCHER
#DEFS += -DSOS_DEBUG_DFT_LOADER
#DEFS += -DSOS_DEBUG_FLASH
#DEFS += -DDBGMODE -DAODV_DEBUG
DEFS += -DDBGMODE

SRCS += neighbor.c aodv.c ping.c
###################################################
# LOADER OPTIONS
###################################################

include ../../config/Makerules

vpath neighbor.c $(ROOTDIR)/modules/routing/neighbor/
vpath aodv.c $(ROOTDIR)/modules/routing/aodv/
vpath ping.c $(ROOTDIR)/modules/management/ping/
