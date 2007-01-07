
PROJ = sos_tool
ROOTDIR = ../..

ADDRESS = 0x8000

#DEFS += -DSOS_DEBUG_CODEMEM
#DEFS += -DSOS_DEBUG_FETCHER
#DEFS += -DSOS_DEBUG_DFT_LOADER
DEFS += -DSOS_DEBUG_FLASH
DEFS += -DDBGMODE

SRCS += loader_pc.c 
###################################################
# LOADER OPTIONS
###################################################

include ../Makerules

clean2:
	rm -f *.sos

vpath loader_pc.c $(ROOTDIR)/modules/management/loader/


