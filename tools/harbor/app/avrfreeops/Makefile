# -*-Makefile-*- #

# Path to the sources
VPATH += ../../lib

# Include files
INCDIR += -I../../lib


PROJ = avrfreeops

# Sources
SRCS += avrbinparser.c avrinstmatch.c avrregs.c dispinstr.c $(PROJ).c

# Objects
OBJS += $(SRCS:.c=.o)

# Platform Specific Settings
HOST = $(shell uname -s)
ifeq ($(HOST), Darwin)
CFLAGS += -DBBIG_ENDIAN
else
CFLAGS += -DLLITTLE_ENDIAN
endif

CFLAGS += -g



%.o : %.c
	gcc $(INCDIR) $(CFLAGS) -c $< -o $@

%.exe : $(OBJS)
	gcc $(OBJS) -o $@

all:	$(PROJ).exe






clean:
	rm -f *.exe *.o *~