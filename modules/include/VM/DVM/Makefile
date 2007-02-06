
# -s generate static wrapper (experimental)
# -n Using sos_read_header_ptr
# -r Don't generate ker_set_current_pid()
JUMPGEN_OPTIONS = -sc
TGR = ../DVMBasiclib.h ../DVMBuffer.h ../DVMConcurrencyMngr.h ../DVMEventHandler.h 
TGR += ../DVMMathlib.h ../DVMResourceManager.h ../DVMScheduler.h ../DVMStacks.h
TGR += ../DVMqueue.h

all: $(TGR)

../%.h : %.h
	sos_jumpgen.exe $(JUMPGEN_OPTIONS) -o $@ $<

clean:
	touch *.h
