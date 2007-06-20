# -*-Makefile-*- #

PROJ = sos_catch
ROOTDIR = ../..

SRCS += $(PROJ).c

# For popping off the tree routing header:
INCDIR += -I$(ROOTDIR)/modules/routing/tree_routing/

# for catch_surge.c and catch_surge_ansi.c:
INCDIR += -I$(ROOTDIR)/modules/demos/surge/

include ../sos_server/lib/Makesossrvlib
