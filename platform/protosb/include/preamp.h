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
 * @brief    header file for preamp tlv2370/ina321
 * @author   Naim Busek
 */


#ifndef _PREAMP_H
#define _PREAMP_H

#define PREAMP_FLAG_BIT 0x80
#define PROTO_AMP_MSK 0x02
#define PREAMP_MSK 0x01

enum {
	PREAMP_INIT=0,
	PREAMP_ON,
	PREAMP_OFF,
};

/**
 * @brief preamp functions
 */
/* active low */
#define preamp_on()		SET_PREAMP_SHDN()
#define preamp_off()	CLR_PREAMP_SHDN()
#define proto_amp_on()	SET_PROTO_SHDN()
#define proto_amp_off()	CLR_PROTO_SHDN()

#define preamp_init()		{ \
	proto_amp_off(); \
	preamp_off(); \
	SET_PREAMP_SHDN_DD_OUT(); \
	SET_PROTO_SHDN_DD_OUT(); \
}

#ifndef _MODULE_
extern int8_t ker_preamp(uint8_t action);
#endif /* _MODULE_ */
#endif

