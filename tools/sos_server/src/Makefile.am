bin_PROGRAMS = sossrv

if ARCH_X86
sossrv_SOURCES = sossrv.c sock_utils.c parsecmd.c dev_serial.c dev_network.c
endif

if ARCH_PPC
sossrv_SOURCES = sossrv.c sock_utils.c parsecmd.c dev_serial_mac.c dev_network.c
sossrv_LDFLAGS = -framework IOKit -framework CoreFoundation 
endif

if ARCH_ARM
sossrv_SOURCES = sossrv.c sock_utils.c parsecmd.c dev_serial.c dev_network.c
endif

INCLUDES = -Iinclude
INCLUDES += -I$(top_srcdir)/../../kernel/include 
INCLUDES += -I$(top_srcdir)/../../modules/include
INCLUDES += -I$(top_srcdir)/../../drivers/include
INCLUDES += -I$(top_srcdir)/../../drivers/uart/include
INCLUDES += -I$(top_srcdir)/../../tools/sos_server/src/include
INCLUDES += -I$(top_srcdir)/../../processor/posix/include 
INCLUDES += -I$(top_srcdir)/../../platform/emu/include

