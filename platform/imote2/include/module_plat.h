/*
 * Copyright (c) 2006 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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
/**
 * @brief    Declare the hardware on the iMote2
 */

#ifndef _MODULE_PLAT_H
#define _MODULE_PLAT_H

#include <kertable_plat.h>

#ifdef _MODULE_

/**
 *  Radio Link Layer Ack
 */
typedef void (*ker_radio_ack_func_t)(void);

/**
 *  Enable Ack 
 */
static inline void ker_radio_ack_enable(void)
{   
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(PROC_KERTABLE_END+1);
	func();
	return;
}

/**
 *  Disable Ack 
 */
static inline void ker_radio_ack_disable(void)
{
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(PROC_KERTABLE_END+2);
	func();
	return;
}  

/**
 * @brief led functions
 * @param action can be following
 *    LED_RED_ON
 *    LED_GREEN_ON
 *    LED_YELLOW_ON
 *    LED_RED_OFF
 *    LED_GREEN_OFF
 *    LED_YELLOW_OFF
 *    LED_RED_TOGGLE
 *    LED_GREEN_TOGGLE
 *    LED_YELLOW_TOGGLE
 */
typedef int8_t (*ledfunc_t)(uint8_t action);

/**
 *  Configure LED 
 */
static inline int8_t ker_led(uint8_t action){
	ledfunc_t func = (ledfunc_t)get_kertable_entry(PROC_KERTABLE_END+3);
	return func(action);
}

#endif //_MODULE_

#endif //_MODULE_PLAT_H
