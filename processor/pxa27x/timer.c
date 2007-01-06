/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License
 *
 *  Copyright (c) 2002 Intel Corporation
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *      Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTEL OR ITS
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */
/* @author Phil Buonadonna
 * @author Robbie Adler
 * @author Andrew Barton-Sweeney
 */

#include <sos_timer.h>
#include <sos.h>
#include "timer.h"
#include <pxa27x_registers.h>
#include <irq.h>

void OSTIrq_fired(void);

/* This implementation of Clock uses the PXA27x OST Channel 5 and only
 * supports the use of TimerM.
 */
uint8_t gmScale;
uint8_t gmInterval;
uint8_t gmCounter;

void OSTIrq_fired()
{
	if (OSSR & OIER_E5) {
		OSSR = (OIER_E5);  // Reset the Status register bit.
		//Clock_fire();			// CALL/SIGNAL TO HIGHER LAYER! (see test.c for now)
		_timer_interrupt();
	}
}

int8_t PXA27XClock_start()
{
	OMCR5 = (OMCR_C | OMCR_P | OMCR_R | OMCR_CRES(0x1));  // Resolution = 1/32768th sec
	//Clock_setRate(mInt,mScl);
	//OSMR5 = 0x8000;				// a binary second match
	OSMR5 = 0x20;					// a binary milisecond match
	OIER |= (OIER_E5);				// Enable the interrupts
	PXA27XIrq_enable(PPID_OST_4_11);
    OSCR5 = 0x0UL;					// Start the counter
    return 0;
}

int8_t PXA27XClock_stop()
{
	OIER &= ~(OIER_E5); // Disable interrupts on channel 5
	PXA27XIrq_disable(PPID_OST_4_11);
    OMCR5 = 0x0UL;  // Disable the counter..
    return 0;
}

#if 0
//int8_t Clock_setRate(uint32_t interval, uint32_t scale)
{
	uint32_t scale = 0;
	//Clock_setInterval(interval);
    uint32_t rate;
	//gmScale = scale;
	//gmInterval = interval;
	//gmCounter = 0;
	PXA27XIrq_allocate(PPID_OST_4_11);
    switch (scale) {
    case 0: rate =  (0 << 0); break;
    case 1: rate =  (1 << 0); break;
    case 2: rate =  (1 << 3); break;
    case 3: rate =  (1 << 5); break;
    case 4: rate =  (1 << 6); break;
    case 5: rate =  (1 << 7); break;
    case 6: rate =  (1 << 8); break;
    default: rate = 0;
    }
    // Set OS Timer Match Register 5 to the given rate
    OMCR5 = (OMCR_C | OMCR_P | OMCR_R | OMCR_CRES(0x1));  // Resolution = 1/32768th sec
    OSMR5 = rate;
	PXA27XIrq_enable(PPID_OST_4_11);
	OIER |= (OIER_E5); // Enable the interrupts
    OSCR5 = 0x0UL;  // Start the counter
    return;
}
#endif

void timer_setInterval(int32_t value)
{
	OIER &= ~(OIER_E5); // Disable interrupts on channel 5
    OSMR5 = value;
    OSCR5 = 0x0;  // start the  counter
	OIER |= (OIER_E5);				// Enable the interrupts
	//gmInterval = value;
    return;
}

/**
 * @brief timer hardware routine
 */
void timer_hardware_init(uint8_t interval, uint8_t scale)
{
	//HAS_CRITICAL_SECTION;
	//ENTER_CRITICAL_SECTION();
	//LEAVE_CRITICAL_SECTION();
	PXA27XIrq_allocate(PPID_OST_4_11,OSTIrq_fired);			// THIS WAS DEFINED IN HPLClock!
	PXA27XClock_start();
	timer_init();
}

/** 
 * Andreas Savvides
 * Enable the watchdog timer to reset processor if system crashes
 *  Set OWER[WME] bit to enable the watchdog
 *  OIER[E3] does not need to be matched to generate an interrupt
 *  OSCR0 is incremented on the rising edge of a 3.25MHz clock
 *  OSMR3 - the OS has to update value of this register before a match
 *          occurs, otherwise the system will reboot
 */
void watchdog_start(uint32_t interval)
{
	uint32_t current_t;
	current_t = OSCR0;
	OSMR3 = (current_t + interval);	
	OWER |= (OWER_WME); // Enable the interrupts
}

/*
 * call this to prevent the watchdog from firing
 */
void watchdog_restart(uint32_t interval)
{
	uint32_t current_t;
	current_t = OSCR0;
	OSMR3 = (current_t + interval);	
}

