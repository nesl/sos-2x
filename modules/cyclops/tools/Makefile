# -*-Makefile-*- #

PROJ = get_cyclops_image
# Set this to the root of the SOS distribution
ROOTDIR = ../../..

VPATH += $(ROOTDIR)/tools/sos_server/lib
VPATH += $(ROOTDIR)/tools/sos_server/src

SRCS += $(PROJ).c sossrv_client.c sock_utils.c

OBJS += $(SRCS:.c=.o)

INCDIR += -I$(ROOTDIR)/tools/sos_server/src/include
INCDIR += -I$(ROOTDIR)/tools/sos_server/lib/include 
INCDIR += -I$(ROOTDIR)/drivers/include
INCDIR += -I$(ROOTDIR)/kernel/include
INCDIR += -I$(ROOTDIR)/modules/include
INCDIR += -I$(ROOTDIR)/processor/posix/include
INCDIR += -I$(ROOTDIR)/platform/cyclops/include
INCDIR += -I$(ROOTDIR)

LDFLAGS += -lm

DEFS += -D_SOS_SERVER_CLIENT_ -DNO_SOS_UART -DNO_SOS_I2C
DEFS += -DNO_SOS_RADIO -DNO_PID_PROC -DNO_PID_PLAT
CFLAGS += $(DEFS) 
LIBS += -lpthread
CC = gcc

# Resolve endian-ness
ifeq ($(MAKECMDGOALS), x86)
CFLAGS += -DLLITTLE_ENDIAN
endif

ifeq ($(MAKECMDGOALS), ppc)
CFLAGS += -DBBIG_ENDIAN
endif

%.o : %.c
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@


%.exe: $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@

all:
	@echo "make {x86|ppc}"

x86: $(PROJ).exe
ppc: $(PROJ).exe


clean:
	rm -fr *~ *.o $(PROJ).exe

