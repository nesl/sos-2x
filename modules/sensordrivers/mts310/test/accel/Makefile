PROJ = accel_app
ROOTDIR = ../../../../..

#DEFS += -DMICASB
SRCS += accel_test_app.c accel_sensor.c loader.c 
INCDIR += -I$(ROOTDIR)/modules/sensordrivers/mts310/include/
SB = mts310

include $(ROOTDIR)/config/Makerules

vpath loader.c $(ROOTDIR)/modules/management/loader/
vpath accel_test_app.c ../../test_modules/accel/
vpath accel_sensor.c ../../accel/