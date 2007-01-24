
PROJ = rats_app
ROOTDIR = ../../..
SRCS += rats.c
SRCS += linear.c
SRCS += test_rats.c
include $(ROOTDIR)/config/Makerules

LDFLAGS += -lm

%.o : $(ROOTDIR)/extensions/rats/%.c
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@ 
%.o : %.c
	$(CC) -c $(CFLAGS) $(INCDIR) $< -o $@ 
