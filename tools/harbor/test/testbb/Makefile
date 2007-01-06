# -*-Makefile-*- #
###################################################
# Brief: Makefile for Test Basic Block
# Author: Ram Kumar {ram@ee.ucla.edu}
###################################################

###################################################
# PROJECT SETUP
###################################################
PROJ = testbb
VPATH += ../../lib
INCDIR += -I../../lib
SRCS += avrregs.c avrinstmatch.c avrbinparser.c basicblock.c dispinstr.c $(PROJ).c
OBJS += $(SRCS:.c=.o)

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
CFLAGS += -g


###################################################
# PROTOTYPES
###################################################
%.o : %.c
	$(CC) $(INCDIR) $(CFLAGS) -c $< -o $@

%.exe : $(OBJS)
	$(CC) $(OBJS) -o $@

all:	$(PROJ).exe


clean:
	rm -f *.exe *.o *~