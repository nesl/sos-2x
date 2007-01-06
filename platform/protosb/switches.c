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
 * @brief    switch module for protosb
 * @author   Naim Busek (ndbusek@gmail.com)
 * @version  0.1
 *
 */

#include <sos.h>
#include <hardware.h>

#include "switches.h"

int8_t ker_switches(uint8_t action){
	  
	if ( action &  SWITCHES_FLAG_BIT) {
		PORTC |= (0xf0 & (action<<4));  // less readable but more efficent
		return SOS_OK; 
	}

	switch (action){
		case SWITCH0_ON: { switch0_on(); break; }
		case SWITCH0_OFF: { switch0_off(); break; }
		
		case SWITCH1_ON: { switch1_on(); break; }
		case SWITCH1_OFF: { switch1_off(); break; }
		
		case SWITCH2_ON: { switch2_on(); break; }
		case SWITCH2_OFF: { switch2_off(); break; }
											
		case SWITCH3_ON: { switch3_on(); break; }
		case SWITCH3_OFF: { switch3_off(); break; }
											
		case SWITCHES_INIT: { switches_init(); break; }
		default:
				switches_init();
				return -EINVAL;
	}
	return SOS_OK;
}
