#!/bin/bash

all:
	cp ../DVMBuffer.c .
	cp ../DVMStacks.c .
	cp ../DVMMathlib.c .
	cp ../DVMqueue.c .
	cp ../DVMResourceManager.c .
	cp ../DVMEventHandler.c .
	cp ../DVMBasiclib.c .
	cp ../DVMScheduler.c .
	cp ../DVMConcurrencyMngr.c .
	make -f buffer.mak mica2
	make -f stacks.mak mica2
	make -f mathlib.mak mica2
	make -f queue.mak mica2
	make -f res.mak mica2
	make -f event.mak mica2
	make -f basiclib.mak mica2 
	make -f sched.mak mica2 
	make -f concurr.mak mica2 
	avr-size -A *.elf


sfi:
	cp ../DVMBuffer.c .
	cp ../DVMStacks.c .
	cp ../DVMMathlib.c .
	cp ../DVMqueue.c .
	cp ../DVMResourceManager.c .
	cp ../DVMEventHandler.c .
	cp ../DVMBasiclib.c .
	cp ../DVMScheduler.c .
	cp ../DVMConcurrencyMngr.c .
	make -f buffer.mak MODE=sfi mica2
	make -f stacks.mak MODE=sfi mica2
	make -f mathlib.mak MODE=sfi mica2
	make -f queue.mak MODE=sfi mica2
	make -f res.mak MODE=sfi mica2
	make -f event.mak MODE=sfi mica2
	make -f basiclib.mak MODE=sfi mica2 
	make -f sched.mak MODE=sfi mica2 
	make -f concurr.mak MODE=sfi mica2 
	avr-size -A *.elf


check:
	echo Checking Buffer
	make -f buffer.mak check
	echo Checking Stats
	make -f stacks.mak check
	echo Checking Mathlib
	make -f mathlib.mak check
	echo Checking Queue
	make -f queue.mak check
	echo Checking Res
	make -f res.mak check
	echo Checking Event
	make -f event.mak check
	echo Checking Basiclib
	make -f basiclib.mak check 
	echo Checking Sched
	make -f sched.mak check 
	echo Checking Concurr
	make -f concurr.mak check 
	

clean:
	rm -f *.c *.o *.s *.srec *.elf *.lst *.i *.i.check *.sos *.map *.sbx *.mlf

