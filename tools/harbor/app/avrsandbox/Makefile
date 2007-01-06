# -*-Makefile-*- #
###################################################
# Brief: Makefile for AVR Sandbox
# Author: Ram Kumar {ram@ee.ucla.edu}
###################################################

###################################################
# PROJECT SETUP
###################################################
PROJ = avrsandbox
VPATH += ../../lib ../../../elfloader/elflib
INCDIR += -I../../lib -I$(SOSROOT)/kernel/include -I../../soslib 
INCDIR += -I../../../elfloader/elflib -I../../../elfloader/minielf
SRCS += avrregs.c avrinstmatch.c avrbinparser.c basicblock.c 
SRCS += sos_mod_header_patch.c dispinstr.c elfdisplay.c elfhelper.c
SRCS += fileutils.c $(PROJ).c
OBJS += $(SRCS:.c=.o)

# For obtaining definition of mod_header_t
#DEFS += -DNO_MOD_PID -DNO_PID_PROC -DNO_PID_PLAT -D_SOS_PC_APP_
#DEFS += -DNO_SOS_UART -DNO_SOS_I2C
#INCDIR += -I$(SOSROOT)/processor/avr/include

###################################################
# PLATFORM SETUP
###################################################
HOST = $(shell uname -s)
ifeq ($(HOST), Darwin)
CFLAGS += -DBBIG_ENDIAN
else
CFLAGS += -DLLITTLE_ENDIAN
endif



###################################################
# COMPILER SETUP
###################################################
CC = gcc -Wall -Werror
CFLAGS += -g $(DEFS)
LDFLAGS +=
LIBS += -lelf

###################################################
# PROTOTYPES
###################################################
%.o : %.c
	$(CC) $(INCDIR) $(CFLAGS) -c $< -o $@

%.exe : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

all:	$(PROJ).exe


clean:
	rm -f *.exe *.o *~