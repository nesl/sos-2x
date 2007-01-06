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
 * @brief    header file for LED debugging
 * @author   Naim Busek
 */


#ifndef _LED_DBG_H
#define _LED_DBG_H

/**
 * need to define these for platforms that do not have leds
 */
#ifndef NO_LEDS
#include <led.h>
#else  // define dummy defs so compiler will not complain before removing them
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
#endif

/**
 * @brief led functions
 */
#if defined(LED_DEBUG) && !defined(NO_LEDS) && !defined(DISABLE_LEDS)
#define LED_DBG(cmd)			ker_led(cmd)
#define SYS_LED_DBG(cmd)        sys_led(cmd)
#define led_dbg_red_on()        led_red_on()
#define led_dbg_green_on()      led_green_on()
#define led_dbg_yellow_on()     led_yellow_on()
#define led_dbg_red_off()       led_red_off()
#define led_dbg_green_off()     led_green_off()
#define led_dbg_yellow_off()    led_yellow_off()
#define led_dbg_red_toggle()    led_red_toggle()
#define led_dbg_green_toggle()  led_green_toggle()
#define led_dbg_yellow_toggle() led_yellow_toggle()
#else
#define LED_DBG(cmd)
#define SYS_LED_DBG(cmd)        
#define led_dbg_red_on()        
#define led_dbg_green_on()     
#define led_dbg_yellow_on()   
#define led_dbg_red_off()       
#define led_dbg_green_off()     
#define led_dbg_yellow_off()    
#define led_dbg_red_toggle()    
#define led_dbg_green_toggle()  
#define led_dbg_yellow_toggle() 
#endif
#define led_dbg_init()			led_init()

#endif

