/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003, Vanderbilt University
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE VANDERBILT UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE VANDERBILT
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE VANDERBILT UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE VANDERBILT UNIVERSITY HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Author: Miklos Maroti
 * Date last modified: 12/07/03
 */

/**
 * @author Simon Han (Port from TinyOS distribution)
 * This module provides a 921.6 KHz timer on the MICA2 platform,
 * and 500 KHz timer on the MICA2DOT platform. We use 1/8 prescaling.
 */
// this field holds the high 16 bits of the current time
#include <sos.h>



union time_u
{
	struct
	{
		uint16_t low;
		uint16_t high;
	};
	uint32_t full;
};


//void systime_init(void);
//void systime_stop(void);
void systime_reset(void);
void systime_event(void);


static volatile uint16_t currentTime;

uint16_t ker_systime16L()
{
	return (uint32_t)get_hvalue(TIMECNT4);			// word or half-word ??
}

uint16_t ker_systime16H()
{
	return currentTime;
}

uint32_t ker_systime32()
{
	return (((uint32_t)currentTime << 16) + (uint32_t)get_hvalue(TIMECNT4));
}

/*
uint32_t ker_systime_castTime16(uint16_t time16)
{
	uint32_t time = call SysTime.getTime32();
	time += (int16_t)time16 - (int16_t)time;
	return time;
}
*/

void systime_init(void)
{
	IRQ_HANDLER_TABLE[INT_TIMER4] = systime_event;		// register timer5 handler
	set_wbit(ILC, ILC_ILC20 & ILC_INT_LV6); 			// set timer5 (IRQ 21) interrupt to level 1
	systime_reset();
}

void systime_stop(void)
{
	return;
}

void systime_reset(void)
{
	put_hvalue(TIMECNTL4, (get_wvalue(TIMECNTL4) & ~(TIMECNTL_IE | TIMECNTL_START)));	// disable interrupt and stop timer4
	currentTime = 0;
	put_hvalue(TIMECNTL4, (TIMECNTL_CLK | TIMECNTL_INT));								// set clksel to CCLK and interval mode
	//put_hvalue(TIMEBASE4, 0);			//(default)										// initialize the base
	//put_hvalue(TIMECNT4, 0);			//(read only)
	//put_hvalue(TIMECMP4, 0xFFFF);		//(default)										// load the timer interval
	put_hvalue(TIMECNTL4, (get_wvalue(TIMECNTL4) | TIMECNTL_IE | TIMECNTL_START));		// enable interrupt and start timer4
}

void systime_event(void)
{
	++currentTime;
	put_hvalue(TIMESTAT4, 0x01);   						// clear the STATUS of the timer4
}
