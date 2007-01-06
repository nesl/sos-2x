# -*-Makefile-*-
#*************************************************
# File: Makefile
# Brief: Build unit-tests for tesing opcode encode/decode
# Author: Ram Kumar {ram@ee.ucla.edu}
#*************************************************
###################################################
# PROJECT SETUP
###################################################
PROJ = opcode_test
VPATH += ../../lib
INCDIR += -I../../lib 
SRCS += avrregs.c avrinstmatch.c $(PROJ).c
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
CFLAGS += -g $(DEFS)


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