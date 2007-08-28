# -*-Makefile-*-
# Testing ALL sensors on Tmote Sky

PROJ = test_sensors_app
MODE = new_sensing_api
SB = tmote
ROOTDIR = ../..
INCDIR += -I$(ROOTDIR)/modules/sensordrivers/include

# Test PAR (Photo-active radiation) Light sensor
SRCS += par_sensor.c 
# Test TSR (Ambient) Light Sensor
SRCS += tsr_sensor.c
# Test SHT1x Temperature and Humidity sensor
SRCS += sht11.c
# Application module to pull sensor data
SRCS += test_sensor.c

include $(ROOTDIR)/config/Makerules

vpath test_sensor.c $(ROOTDIR)/modules/sensordrivers/tmote/new_api/test/
vpath par_sensor.c $(ROOTDIR)/modules/sensordrivers/tmote/new_api/par/
vpath tsr_sensor.c $(ROOTDIR)/modules/sensordrivers/tmote/new_api/tsr/
vpath sht11.c $(ROOTDIR)/modules/sensordrivers/tmote/new_api/sht1x/

clean2:
	rm -f *.o *.srec *.elf *.lst $(PROJ).map $(PROJ).exe $(PROJ).exe.out.* $(PROJ).od $(PROJ).srec.out.* .build.tmp
	rm -f $(PROJ).c Makefile

