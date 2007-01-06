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

/**
 * @brief    LED definitions for Cyclops platform
 * @author   Simon Han
 */


#ifndef _LED_H
#define _LED_H

#define LED_RED_ON           1
#define LED_GREEN_ON         2
#define LED_YELLOW_ON        3
#define LED_RED_OFF          4
#define LED_GREEN_OFF        5
#define LED_YELLOW_OFF       6
#define LED_RED_TOGGLE       7
#define LED_GREEN_TOGGLE     8
#define LED_YELLOW_TOGGLE    9
#define LED_AMBER_ON        10
#define LED_AMBER_OFF       11
#define LED_AMBER_TOGGLE    12


/**
 * @brief led functions
 */
#if !defined(DISABLE_LEDS)
#define led_red_on()        PORTF &= ~(1)
#define led_green_on()      PORTF &= ~(1<<(1))
#define led_yellow_on()     PORTF &= ~(1<<(2))
#define led_amber_on()     PORTF &= ~(1<<(3))
#define led_red_off()       PORTF |= 1
#define led_green_off()     PORTF |= (1<<(1))
#define led_yellow_off()    PORTF |= (1<<(2))
#define led_amber_off()    PORTF |= (1<<(3))
#define led_red_toggle()    PORTF ^= 1
#define led_green_toggle()  PORTF ^= (1 << 1)
#define led_yellow_toggle() PORTF ^= (1 << 2)
#define led_amber_toggle() PORTF ^= (1 << 3)
#define led_init() { \
	DDRF |= 0xF; \
	led_green_off(); \
	led_red_off(); \
	led_yellow_off(); \
	led_amber_off(); \
}
#else 
#define led_red_on()        
#define led_green_on()     
#define led_yellow_on()   
#define led_amber_on()   
#define led_red_off()       
#define led_green_off()     
#define led_yellow_off()    
#define led_amber_off()    
#define led_red_toggle()    
#define led_green_toggle()  
#define led_yellow_toggle() 
#define led_amber_toggle() 
#define led_init()     {  DDRF &= ~0x0F; }
#endif

/**
 * kernel writer can just use macros provided by SOS
 *
 * led_red_on()        
 * led_green_on()    
 * led_yellow_on()   
 * led_red_off()      
 * led_green_off()     
 * led_yellow_off()    
 * led_red_toggle() 
 * led_green_toggle() 
 * led_yellow_toggle()
 */
#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_led(uint8_t action);

#endif /* _MODULE_ */
#endif

