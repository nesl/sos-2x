ROOTDIR = $(SOSROOT)
INCDIR += -I$(ROOTDIR)/modules/include
INCDIR += -I$(ROOTDIR)/kernel/include

MACHINE = $(shell uname -m)

ifeq ($(MAKECMDGOALS), x86)
CFLAGS += -DLLITTLE_ENDIAN
endif

ifeq ($(MAKECMDGOALS), ppc)
ifeq ($(MACHINE), i386)
CFLAGS += -DLLITTLE_ENDIAN
else
CFLAGS += -DBBIG_ENDIAN
endif
endif

DEFS += -DNO_PID_PLAT -DNO_PID_PROC

all:
	@echo "Compiling codesend HowTo"
	@echo "Cygwin/Linux PC: make x86"
	@echo "MAC OS X: make ppc"

x86: dvm_compiler 

ppc: dvm_compiler 

dvm_compiler: dvm_compiler.c
	flex dvm_compiler.c
	gcc $(INCDIR) $(CFLAGS) $(DEFS) lex.yy.c -o $@ -lfl
	rm lex.yy.c

clean:
	rm -fr dvm_compiler
