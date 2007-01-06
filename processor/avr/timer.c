/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 *
 */

#include <sos_timer.h>
#include <sos.h>
#include "timer.h"

/** 
 * @brief timer hardware routine
 */
void timer_hardware_init(uint8_t interval, uint8_t scale){
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();

	scale &= 0x7;
	scale |= (1<<WGM1); // reset on match
	

	TIMSK &= ((unsigned char)~(1 << (TOIE0)));
	TIMSK &= ((unsigned char)~(1 << (OCIE0)));
	//!< Disable TC0 interrupt

	/** 
	 *  set Timer/Counter0 to be asynchronous 
	 *  from the CPU clock with a second external 
	 *  clock(32,768kHz)driving it
	 */
	ASSR |= (1 << (AS0)); //!< us external oscillator
	TCCR0 = scale;
	TCNT0 = 0;
	OCR0 = interval;
	//TIMSK |= (1 << (OCIE0)); replaced by the line below
	timer_enable_interrupt();
	LEAVE_CRITICAL_SECTION();

	timer_init();
}   

void timer_setInterval(int32_t val)
{
	uint8_t interval;
	while ((ASSR & (_BV(OCR0UB) | _BV(TCN0UB))) != 0);

	if(val > 250) {interval = 250;}
	else if(val <= 2) {interval = 2;}
	else {interval = (uint8_t)val - 1;}

	//! reset hardware counter
	TCNT0 = 0;
	OCR0 = interval;
}


