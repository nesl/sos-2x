/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _TIMER_H
#define _TIMER_H
#include <avr/interrupt.h>

#define PROCESSOR_TICKS(x) x

/**
 * @brief Initialize the timer hardware
 * @param interval Hardware timer interval
 * @param scale The hardware pre-scalar setting
 */
extern void timer_hardware_init(uint8_t interval, uint8_t scale);
/**
 * @brief Set a new value for the timer interval
 * @param val The value of the new interval
 */
extern void timer_setInterval(int32_t val);
// Timer Interrupt from hardware
#define timer_interrupt()    SIGNAL(SIG_OUTPUT_COMPARE0)
/**
 * @brief Get the current interval setting
 */ 
static inline uint8_t timer_getInterval()   {return OCR0 + 1;}
/**
 * @brief Get the current value stored in the hardware register
 */
static inline uint8_t timer_hardware_get_counter()
{
	return TCNT0;
}

static inline void timer_disable_interrupt()
{
	TIMSK &= ~(1 << (OCIE0));
}

static inline void timer_enable_interrupt()
{
	TIMSK |= (1 << (OCIE0));
}

#endif // _TIMER_H

