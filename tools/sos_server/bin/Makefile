# -*-Makefile-*- #
PROJ = sossrv
ROOTDIR = ../../..

#PATH TO SOURCE FILES
VPATH += $(ROOTDIR)/tools/sos_server/src

# PATH TO INCLUDE FILES
INCDIR += -I$(ROOTDIR)/kernel/include -I$(ROOTDIR)/modules/include
INCDIR += -I$(ROOTDIR)/drivers/include 
INCDIR += -I$(ROOTDIR)/drivers/uart/include 
INCDIR += -I$(ROOTDIR)/tools/sos_server/src/include 
INCDIR += -I$(ROOTDIR)/processor/posix/include -I$(ROOTDIR)/platform/emu/include


CFLAGS += -DNO_PID_PROC -DNO_PID_PLAT -D_SOS_SERVER_APP_

ifeq ($(MAKECMDGOALS), arm)
SRCS += sossrv.c sock_utils.c parsecmd.c dev_serial.c dev_network.c
CFLAGS += -DLLITTLE_ENDIAN
TRG=arm-linux-
endif

ifeq ($(MAKECMDGOALS), x86)
SRCS += sossrv.c sock_utils.c parsecmd.c dev_serial.c dev_network.c
CFLAGS += -DLLITTLE_ENDIAN
TRG=
endif

ifeq ($(MAKECMDGOALS), ppc)
SRCS += sossrv.c sock_utils.c parsecmd.c dev_serial_mac.c dev_network.c
CFLAGS += -DBBIG_ENDIAN
LDFLAGS += -framework IOKit -framework CoreFoundation
TRG=
endif

# OBJECT FILES
OBJS += $(SRCS:.c=.o)

# C COMPILER
CC = $(TRG)gcc



%.o : %.c
	@echo "-------------------------------"
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@

%.exe: $(OBJS)
	@echo "-------------------------------"
	@echo "Linking ..."
	$(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $@




all:
	@echo "Compiling sossrv HowTo"
	@echo "Cygwin/Linux PC: make x86"
	@echo "MAC OS X: make ppc"
	@echo "ARM Linux: make arm"


x86: $(PROJ).exe
	@echo "Compiling sos_srv for x86 target"

ppc: $(PROJ).exe
	@echo "Compiling sos_srv for PPC target"

arm: $(PROJ).exe
	@echo "Compiling sos_srv for ARM target"

clean:
	@echo "Cleaning up ..."
	rm -fr *.o *~ sossrv.exe $(ROOTDIR)/src/*~ $(ROOTDIR)/src/include/*~


