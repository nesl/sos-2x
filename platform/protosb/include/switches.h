/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
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
 * @brief    header file for voltage reference
 * @author   Naim Busek
 */


#ifndef _SWITCHES_H
#define _SWITCHES_H

#define SWITCHES_FLAG_BIT	0x80

enum {
	SWITCHES_INIT=0,
	SWITCH0_ON,
	SWITCH0_OFF,
	SWITCH1_ON,
	SWITCH1_OFF,
	SWITCH2_ON,
	SWITCH2_OFF,
	SWITCH3_ON,
	SWITCH3_OFF,
};

/**
 * @brief switches functions
 */
/* active low */
#define switch0_on()		CLR_PWR_CTRL0()
#define switch0_off()		SET_PWR_CTRL0()
#define switch1_on()		CLR_PWR_CTRL1()
#define switch1_off()		SET_PWR_CTRL1()
#define switch2_on()		CLR_PWR_CTRL2()
#define switch2_off()		SET_PWR_CTRL2()
#define switch3_on()		CLR_PWR_CTRL3()
#define switch3_off()		SET_PWR_CTRL3()

#define switches_init()		{ \
	switch0_off(); \
	switch1_off(); \
	switch2_off(); \
	switch3_off(); \
	SET_PWR_CTRL0_DD_OUT(); \
	SET_PWR_CTRL1_DD_OUT(); \
	SET_PWR_CTRL2_DD_OUT(); \
	SET_PWR_CTRL3_DD_OUT(); \
}

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_switches(uint8_t action);
#endif /* _MODULE_ */
#endif

