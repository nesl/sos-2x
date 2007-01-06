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
#include <hardware_types.h>
#include <hardware_proc.h>
#include <sos_types.h>
#include <measurement.h>
#include <systime.h>

void WakeupTimer_fired();

// this field holds the high 16 bits of the current time
union time_u
{
	struct
	{
		uint16_t low;
		uint16_t high;
	};
	uint32_t full;
};

static volatile uint16_t currentTime;

uint16_t ker_systime16L()
{
	return TCNT3;
}

uint16_t ker_systime16H()
{
	return currentTime;
}

uint32_t ker_systime32()
{
	register union time_u time;
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();
	{
		time.low = TCNT3;
		time.high = currentTime;

		// maybe there was a pending interrupt
		if( bit_is_set(ETIFR, TOV3) && ((int16_t)time.low) >= 0 )
			++time.high;
	}
	LEAVE_CRITICAL_SECTION();

	return time.full;
}

/*
uint32_t ker_systime_castTime16(uint16_t time16)
{
	uint32_t time = call SysTime.getTime32();
	time += (int16_t)time16 - (int16_t)time;
	return time;
}
*/

// Use SIGNAL instead of INTERRUPT to get atomic update of time
SIGNAL(SIG_OVERFLOW3)
{
	SOS_MEASUREMENT_IDLE_END();
	++currentTime;
	if(currentTime == TIMER3_MAX_GTIME)
		currentTime = 0;	
	
	#ifdef UBMAC
	WakeupTimer_fired();		
	#endif
}

void systime_init()
{
	uint8_t etimsk;
	HAS_CRITICAL_SECTION;

	TCCR3A = 0x00;
	TCCR3B = 0x00;

	ENTER_CRITICAL_SECTION();
	{
		etimsk = ETIMSK;
		etimsk &= (1<<OCIE1C);
		etimsk |= (1<<TOIE3);
		ETIMSK = etimsk;
	}
	LEAVE_CRITICAL_SECTION();
	//! start the timer
	//! start the timer with 1/64 prescaler, 115.2 KHz on MICA2
	TCCR3B = 0x03;
}

void systime_stop()
{
	// stop the timer
	TCCR3B = 0x00;
}
