/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
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
*
*/
/**
* Low level hardware access to the CC1000
* @author Jaein Jeong
* @author Philip Buonadonna
*
* @author Simon Han
*   Port to SOS 
*/

#include "hardware.h"

void cc1k_init(){
	SET_CC_CHP_OUT_DD_IN();
	SET_CC_PALE_DD_OUT();  // PALE
	SET_CC_PCLK_DD_OUT();    // PCLK
	SET_CC_PDATA_DD_OUT();    // PDATA
	SET_CC_PALE();      // set PALE high
	SET_CC_PDATA();        // set PCLK high
	SET_CC_PCLK();        // set PDATA high
}

void cc1k_write(uint8_t addr, uint8_t data){
	char cnt = 0;

	// address cycle starts here
	addr <<= 1;
	CLR_CC_PALE();  // enable PALE
	for (cnt=0;cnt<7;cnt++)  // send addr PDATA msb first
	{
		if (addr&0x80) {
			//SET_CC_PDATA();
			asm volatile ("sbi 0x12, 7" ::);
		} else {
			//CLR_CC_PDATA();
			asm volatile ("cbi 0x12, 7" ::);
		}
		CLR_CC_PCLK();   // toggle the PCLK
		SET_CC_PCLK();
		addr <<= 1;
	}
	SET_CC_PDATA();
	CLR_CC_PCLK();   // toggle the PCLK
	SET_CC_PCLK();

	SET_CC_PALE();  // disable PALE

	// data cycle starts here
	for (cnt=0;cnt<8;cnt++)  // send data PDATA msb first
	{
		if (data&0x80) {
			//SET_CC_PDATA();
			asm volatile ("sbi 0x12, 7" ::);
		} else {
			//CLR_CC_PDATA();
			asm volatile ("cbi 0x12, 7" ::);
		}
		CLR_CC_PCLK();   // toggle the PCLK
		SET_CC_PCLK();
		data <<= 1;
	}
	SET_CC_PALE();
	SET_CC_PDATA();
	SET_CC_PCLK();

}

uint8_t cc1k_read(uint8_t addr){
	int cnt;
	uint8_t din;
	uint8_t data = 0;

	// address cycle starts here
	addr <<= 1;
	CLR_CC_PALE();  // enable PALE
	for (cnt=0;cnt<7;cnt++)  // send addr PDATA msb first
	{
		if (addr&0x80) {
			//SET_CC_PDATA();
			asm volatile ("sbi 0x12, 7" ::);
		} else {
			//CLR_CC_PDATA();
			asm volatile ("cbi 0x12, 7" ::);
		}
		CLR_CC_PCLK();   // toggle the PCLK
		SET_CC_PCLK();
		addr <<= 1;
	}
	//CLR_CC_PDATA();
	asm volatile ("cbi 0x12, 7" ::);
	CLR_CC_PCLK();   // toggle the PCLK
	SET_CC_PCLK();

	SET_CC_PDATA_DD_IN();  // read data from chipcon
	SET_CC_PALE();  // disable PALE

	// data cycle starts here
	for (cnt=7;cnt>=0;cnt--)  // send data PDATA msb first
	{
		CLR_CC_PCLK();  // toggle the PCLK
		din = READ_CC_PDATA();
		if(din)
			data = (data<<1)|0x01;
		else
			data = (data<<1)&0xfe;
		SET_CC_PCLK();
	}

	SET_CC_PALE();
	SET_CC_PDATA_DD_OUT();
	SET_CC_PDATA();

	return data;
}

bool cc1k_getLock(){
	char cVal;

	cVal = READ_CC_CHP_OUT();

	return cVal;
}



