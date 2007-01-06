/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*									tab:4
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
 */

/* 
 * Authors: Jaein Jeong, Philip buonadonna
 * Simon Han: port to SOS
 *
 */

/**
 * @author Jaein Jeong
 * @author Philip buonadonna
 */
#include <hardware.h>
#include <measurement.h>
#include <message.h>


static uint8_t OutgoingByte; 

SIGNAL (SIG_SPI) {
	register uint8_t temp; 
	SOS_MEASUREMENT_IDLE_END();
	temp = SPDR;
	SPDR = OutgoingByte;
	cc1k_radio_spi_interrupt(temp);
}

void spi_writeByte(uint8_t data){
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	OutgoingByte = data;
	LEAVE_CRITICAL_SECTION();
}

void spi_init(){
	SET_SCK_DD_IN();
	SET_MISO_DD_IN(); // miso
	SET_MOSI_DD_IN(); // mosi
	SPCR &= ~(1<<(CPOL));    // Set proper polarity...
	SPCR &= ~(1<<(CPHA));    // ...and phase
	SPCR |= (1<<(SPIE));  // enable spi port
	SPCR |= (1<<(SPE));
}


