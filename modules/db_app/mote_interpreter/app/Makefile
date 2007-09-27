PROJ = inter_app

ROOTDIR = ${SOSROOT}

SRCS += loader.c neighbor.c tree_routing.c accel_sensor.c interpreter.c
#SRCS += mag_sensor.c phototemp_sensor.c

INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/ -I../modules
DEFS += -DMTS310SB
SB = mts310

include $(ROOTDIR)/config/Makerules

vpath loader.c $(ROOTDIR)/extensions/loader/
vpath neighbor.c $(ROOTDIR)/modules/routing/neighbor/
vpath tree_routing.c $(ROOTDIR)/modules/routing/tree_routing/
ifeq ($(PROCESSOR), posix)
  vpath accel_sensor.c $(HOME)/saf-netcar/trunk/sos/modules/
else
  vpath accel_sensor.c $(ROOTDIR)/modules/sensordrivers/mts310/accel/
endif 
#vpath mag_sensor.c $(ROOTDIR)/modules/sensordrivers/mts310/mag/
#vpath phototemp_sensor.c $(ROOTDIR)/modules/sensordrivers/mts310/photo_temp/
vpath interpreter.c $(ROOTDIR)/modules/db_app/mote_interpreter/
