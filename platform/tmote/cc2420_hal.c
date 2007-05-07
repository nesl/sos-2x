/* "Copyright (c) 2000-2003 The Regents of the University  of California.  
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
 * @file cc2420_hal.c
 * @brief CC2420 Hardware Abstraction Layer
 * @author hubert wu {hubert@cs.ucla.edu}
 */

//--------------------------------------------------------
// INCLUDES
//--------------------------------------------------------
#include <hardware.h>
#include <spi_hal.h>
#include <cc2420_hal.h>
//#define LED_DEBUG
#include <led_dbg.h>

uint8_t _current_stat=0;					//current stat
void (*_FIFOP_IS_SET_CALL)(int16_t timestamp) = NULL;	//FIFOP callback function
void (*_FIFO_IS_SET_CALL)(int16_t timestamp) = NULL;	//FIFO callback function
void (*_CCA_IS_SET_CALL)(int16_t timestamp) = NULL;	//CCA callback function
void (*_SFD_IS_SET_CALL)(int16_t timestamp) = NULL;	//SFD is set callback function
void (*_SFD_IS_CLR_CALL)(int16_t timestamp) = NULL;	//SFD is clear callback function

/*****************************************************************
 * Wait for million second                                       *
 *****************************************************************/
void TC_UWAIT(uint16_t u)
{
  uint16_t i;
  if (u < 500)
    for (i=2; i < u; i++) {
      asm volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   ::);
    }
  else
    for (i=0; i < u; i++) {
      asm volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   ::);
    }
}

/*****************************************************************
 * Wait for more time                                            *
 *****************************************************************/
void TC_MWAIT(uint16_t u)
{
	uint16_t i;
	for(i=0; i<u; i++) {
		TC_UWAIT(u);
	}
}

/*****************************************************************
 * Initiate pin directions                                       *
 *****************************************************************/
void TC_INIT_PINS() 
{
}

/*****************************************************************
 * Write byte to spi                                             *
 *****************************************************************/
void TC_WRITE_BYTE(uint8_t CC)
{	
	int j;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	TC_SET_SPI_WRITE;
	for(j=7; j>=0; j--) {
		if( GETBIT(CC,j) )
			TC_SET_SI;	
		else
			TC_CLR_SI;
		TC_SET_SCLK;	//generate the clock for spi
		TC_CLR_SCLK;	//generate the clock for spi
	}
	LEAVE_CRITICAL_SECTION();
}

/*****************************************************************
 * Write word to spi                                             *
 *****************************************************************/
void TC_WRITE_WORD(uint16_t CC)
{	
	int j;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	TC_SET_SPI_WRITE;
	for(j=15; j>=0; j--) {
		if( GETBIT(CC,j) )
			TC_SET_SI;	
		else
			TC_CLR_SI;
		TC_SET_SCLK;
		TC_CLR_SCLK;
	}
	LEAVE_CRITICAL_SECTION();
}

/*****************************************************************
 * Read byte from spi                                            *
 *****************************************************************/
void TC_READ_BYTE(uint8_t *CC)
{	
	int j;
	uint8_t ret = 0;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	TC_SET_SPI_READ;
	for(j=7; j>=0; j--) {
		TC_SET_SCLK;		//generate the clock for spi
		if( TC_SO_IS_SET )
			SETBITHIGH(ret,j);
		TC_CLR_SCLK;		//generate the clock for spi
	}
	*CC = ret;
	LEAVE_CRITICAL_SECTION();
}

/*****************************************************************
 * Read word from spi                                            *
 *****************************************************************/
void TC_READ_WORD(uint16_t *CC)
{	
	int j;
	uint16_t ret = 0;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	TC_SET_SPI_READ;
	for(j=15; j>=0; j--) {
		TC_SET_SCLK;
		if( TC_SO_IS_SET )
			SETBITHIGH(ret,j);
		TC_CLR_SCLK;
	}
	*CC = ret;
	LEAVE_CRITICAL_SECTION();
}

/*****************************************************************
 * read register word                                            *
 *****************************************************************/
void TC_GET_REG(uint8_t R, int8_t I, int8_t J, uint16_t* CC)
{
	int8_t i=I;
	int8_t j=J;
	TC_UNSELECT_RADIO;
	TC_SELECT_RADIO;
	TC_WRITE_BYTE((0x40|R));
	TC_READ_WORD(CC);
	*CC = GETBITS(*CC,i,j);
	TC_UNSELECT_RADIO;
	TC_SELECT_RADIO;
}


/*****************************************************************
 * set register word                                             *
 *****************************************************************/
void TC_SET_REG(uint8_t R, int8_t I, int8_t J, uint16_t CC)
{
	uint16_t wd;
	uint16_t tmp;
	int8_t i=I;
	int8_t j=J;
	TC_UNSELECT_RADIO;
	TC_SELECT_RADIO;
	TC_WRITE_BYTE((0x40|R));
	TC_READ_WORD(&wd);
	tmp=SETBITS(wd,CC,i,j);
	TC_WRITE_BYTE(R);
	TC_WRITE_WORD(wd);
	TC_UNSELECT_RADIO;
	TC_SELECT_RADIO;
}


/*****************************************************************
 * run a strobe command on cc2420                                *
 *****************************************************************/
void TC_STROBE(uint8_t CC)
{	
	TC_CLR_CSN;
	TC_WRITE_BYTE(CC);
	TC_READ_BYTE(&_current_stat);
	TC_SET_CSN;
	TC_CLR_CSN;
}

/*****************************************************************
 * set callback functions                                        *
 *****************************************************************/
void TC_SetFIFOPCallBack(void (*f)(int16_t timestamp)) {
	_FIFOP_IS_SET_CALL = f;
}
void TC_SetFIFOCallBack(void (*f)(int16_t timestamp)) {
	_FIFO_IS_SET_CALL = f;
}
void TC_SetCCACallBack(void (*f)(int16_t timestamp)) {
	_CCA_IS_SET_CALL = f;
}
void TC_SetSFDCallBack(void (*f1)(int16_t timestamp), void (*f2)(int16_t timestamp)) {
	_SFD_IS_SET_CALL = f1;
	_SFD_IS_CLR_CALL = f2;
}

/*****************************************************************
 * define the interrupt functio                                  *
 *****************************************************************/
interrupt(PORT1_VECTOR) Port1_Interrupt(void) {
	int16_t timestamp;
	static int16_t st_timestamp;		//remember the time when start receiving packet
	timestamp = getTime();
	if(TC_FIFOP_IFG_IS_SET) {
		if(_FIFOP_IS_SET_CALL) 
//			_FIFOP_IS_SET_CALL(timestamp);
			_FIFOP_IS_SET_CALL(st_timestamp); //we want the timestamp when it receives the first byte
		TC_FLUSH_RX;
		TC_CLR_FIFOP_IFG;
	}
	else if(TC_FIFO_IFG_IS_SET) {
		st_timestamp = getTime();
		if(_FIFO_IS_SET_CALL) 
			_FIFO_IS_SET_CALL(timestamp);
		TC_CLR_FIFO_IFG;
	}
	else if(TC_CCA_IFG_IS_SET) {
		if(_CCA_IS_SET_CALL) 
			_CCA_IS_SET_CALL(timestamp);
		TC_CLR_CCA_IFG;
	}
}


/*****************************************************************
 * define endian switch function for host between net            *
 *****************************************************************/
uint16_t host_to_net(uint16_t a)
{
	return a;
}

uint16_t net_to_host(uint16_t a)
{
	return a;
}

