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
 * @brief    header file for voltage regulators max1776
 * @author   Naim Busek
 */


#ifndef _VREG_H
#define _VREG_H

#define VREG_FLAG_BIT		0x80

enum {
	VREG_INIT=0,
	VREG0_ON,
	VREG0_OFF,
	VREG0_TOGGLE,
	VREG1_ON,
	VREG1_OFF,
	VREG1_TOGGLE,
};

/**
 * @brief vreg functions
 */
/* active low */
#define vreg0_on()		SET_EXT_PWR_EN0()
#define vreg0_off()		CLR_EXT_PWR_EN0()
#define vreg0_toggle() TOGGLE_EXT_PWR_EN0()
#define vreg1_on()		SET_EXT_PWR_EN1()
#define vreg1_off()		CLR_EXT_PWR_EN1()
#define vreg1_toggle() TOGGLE_EXT_PWR_EN1()

#define vreg_init()		{ \
	vreg0_off(); \
	vreg1_off(); \
	SET_V_SEL0_DD_OUT(); \
	SET_V_SEL1_DD_OUT(); \
	SET_V_SEL0(); \
	SET_V_SEL1(); \
	SET_EXT_PWR_EN0_DD_OUT(); \
	SET_EXT_PWR_EN1_DD_OUT(); \
}

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_vreg(uint8_t action);
#endif /* _MODULE_ */
#endif

