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
 * @brief CC2420 Hardware Abstraction Layer for imote2
 * @author hubert wu {hubert@cs.ucla.edu}
 * @author Andrew Barton-Sweeney {abs@cs.yale.edu}
 * @author Dimitrios Lymberopoulos {dimitrios.lymberopoulos@yale.edu}
 */

//--------------------------------------------------------
// INCLUDES
//--------------------------------------------------------
#include <hardware.h>
//#include <spi_hal.h>
#include <cc2420_hal.h>
//#define LED_DEBUG
#include <led_dbg.h>

#define DEASSERT_SPI_CS {TOSH_uwait(1); SOS_SET_CC_CSN_PIN();}
#define ASSERT_SPI_CS {SOS_CLR_CC_CSN_PIN(); TOSH_uwait(1);}
//#define DRAIN_RXFIFO(_tmp) {while (SSSR_3 & SSSR_RNE) _tmp = SSDR_3;}

// Enables/disables the SPI interface
#define SPI_ENABLE()        do { GPCR1 |= 0x00000080; TOSH_uwait(1); } while (0)
#define SPI_DISABLE()       do { TOSH_uwait(1); GPSR1 |= 0x00000080; } while (0)

/* Wait till Busy bit in SSP status register is cleared */
#define 	SSIO_WAIT_BUSY		while ((SSSR_3 & SSSR_BSY) == SSSR_BSY)

uint8_t _current_stat=0;					//current stat
static int16_t st_timestamp;		//remember the time when start receiving packet

void fifop_interrupt(void);
void fifo_interrupt(void);

void (*_FIFOP_IS_SET_CALL)(int16_t timestamp) = NULL;	//FIFOP callback function
void (*_FIFO_IS_SET_CALL)(int16_t timestamp) = NULL;	//FIFO callback function
void (*_CCA_IS_SET_CALL)(int16_t timestamp) = NULL;	//CCA callback function
void (*_SFD_IS_SET_CALL)(int16_t timestamp) = NULL;	//SFD is set callback function
void (*_SFD_IS_CLR_CALL)(int16_t timestamp) = NULL;	//SFD is clear callback function

/*****************************************************************
 * Wait for million second                                       *
 *****************************************************************/
#if 0
void TC_UWAIT(uint16_t u)
{
	uint16_t i;
	for (i=0; i < u; i++) {
		asm volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
		   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
 		   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   ::);
	}
}
#endif

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
	
  SOS_SET_CC_RSTN_PIN();
  SOS_CLR_CC_VREN_PIN();
  SOS_SET_CC_CSN_PIN();

  SOS_MAKE_CC_RSTN_OUTPUT();
  SOS_MAKE_CC_VREN_OUTPUT();
  SOS_MAKE_CC_CSN_OUTPUT();
  SOS_MAKE_CC_FIFOP_INPUT();
  SOS_MAKE_CC_FIFO_INPUT();
  SOS_MAKE_CC_SFD_INPUT();
  SOS_MAKE_RADIO_CCA_INPUT();

  /* Hardware SPI Initalization */
  /* Setup the SSP functionality on the required pins*/
	_PXA_setaltfn(SSP3_SCLK,SSP3_SCLK_ALTFN,GPIO_OUT);
	_PXA_setaltfn(SSP3_TXD,SSP3_TXD_ALTFN,GPIO_OUT);
	_PXA_setaltfn(SSP3_RXD,SSP3_RXD_ALTFN,GPIO_IN);
	/*_PXA_setaltfn(SSP3_SFRM,SSP3_SFRM_ALTFN,GPIOIN);*/
	/* Disable the SSP port before configuration */
	SSCR0_3 &= ~(SSCR0_SSE);
	CKEN &= ~(CKEN4_SSP3);
	/* Configure SSP */
	SSCR0_3 = ( SSCR0_TIM | SSCR0_RIM ); /* Disable underflow/overflow interrupts */
	SSCR0_3 = SSCR0_SCR(1); /* 13MHz/2 bit rate */
	SSCR0_3 = SSCR0_FRF(0); /* Motorola SPI Mode */
	SSCR0_3 = SSCR0_DSS(7); /* Set TX/RX data size to 8 bits */
	/*SSCR1_3 = SSCR1_TTE; // Do not drive the TX line when not transmitting */
	SSCR1_3 = SSCR1_RFT(0) | SSCR1_TFT(0); /* TXFIFO and RXFIFO thresholds */
	SSTO_3 = (96*1); /* NOT USED: Timer inteerrupt for RXFIFO is disabled!*/
	/*Enable the SSP port*/
	CKEN |= CKEN4_SSP3;
	SSCR0_3 |= SSCR0_SSE;
}

/*****************************************************************
 * Write byte to spi                                             *
 *****************************************************************/
void TC_WRITE_BYTE(uint8_t CC)
{
	uint8_t tmp;
	SSIO_WAIT_BUSY;
	SSDR_3 = (uint8_t) CC;
	SSIO_WAIT_BUSY;
	tmp = (uint8_t) SSDR_3;
}

/*****************************************************************
 * Write byte to spi                                             *
 *****************************************************************/
void TC_WRITE_BYTE_NO_READ(uint8_t CC)
{
	SSIO_WAIT_BUSY;
	SSDR_3 = (uint8_t) CC;
	SSIO_WAIT_BUSY;
}

/*****************************************************************
 * Write word to spi                                             *
 *****************************************************************/
void TC_WRITE_WORD(uint16_t CC)
{
	uint8_t tmp;
	SSIO_WAIT_BUSY;
	SSDR_3 = (uint8_t) (CC >> 8);
	SSIO_WAIT_BUSY;
	tmp = (uint8_t) SSDR_3;
	SSIO_WAIT_BUSY;
	SSDR_3 = (CC);
	SSIO_WAIT_BUSY;
	tmp = (uint8_t) SSDR_3;
}

/*****************************************************************
 * Read byte from spi                                            *
 *****************************************************************/
void TC_READ_BYTE(uint8_t *CC)
{
	SSIO_WAIT_BUSY;
	SSDR_3 = CC2420_SNOP;
	SSIO_WAIT_BUSY;
	*CC = (uint8_t) SSDR_3;
}

/*****************************************************************
 * Read word from spi                                            *
 *****************************************************************/
void TC_READ_WORD(uint16_t *CC)
{
	uint8_t temp;
	SSIO_WAIT_BUSY;
	SSDR_3 = CC2420_SNOP;
	SSIO_WAIT_BUSY;
	temp = (uint8_t) SSDR_3;
	*CC = (uint16_t) ((temp << 8) & 0xFF00);
	SSIO_WAIT_BUSY;
	SSDR_3 = CC2420_SNOP;
	SSIO_WAIT_BUSY;
	temp = (uint8_t) SSDR_3;
	*CC = (*CC | ((uint16_t) temp));
}

/*****************************************************************
 * read register word                                            *
 *****************************************************************/
void TC_GET_REG(uint8_t R, int8_t I, int8_t J, uint16_t* C)
{
	int8_t i=I;
	int8_t j=J;
	TC_SELECT_RADIO;
	TC_WRITE_BYTE((0x40|R));
	TC_READ_WORD(C);
	*C = GETBITS(*C,i,j);
	TC_UNSELECT_RADIO;
}


/*****************************************************************
 * set register word                                             *
 *****************************************************************/
void TC_SET_REG(uint8_t R, int8_t I, int8_t J, uint16_t C)
{
	uint16_t wd;
	uint16_t tmp;
	int8_t i=I;
	int8_t j=J;
	TC_SELECT_RADIO;
	TC_WRITE_BYTE((0x40|R));
	TC_READ_WORD(&wd);
	tmp=SETBITS(wd,C,i,j);
	TC_WRITE_BYTE(R);
	TC_WRITE_WORD(wd);
	TC_UNSELECT_RADIO;
}

/*****************************************************************
 * run a strobe command on cc2420                                *
 *****************************************************************/
void TC_STROBE(uint8_t CC)
{
	TC_CLR_CSN;
	TC_WRITE_BYTE_NO_READ(CC);
	_current_stat = (uint8_t) SSDR_3;
	TC_SET_CSN;
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
void TC_Setup_Interrupt()
{
	// allocate interrupt for FIFOP and FIFO
	// enable FIFO
	// disable FIFOP
	// SFD ???
	//PXA27XGPIOInt_enable(CC_FIFOP_PIN,SOS_RISING_EDGE,fifop_interrupt);
	//PXA27XGPIOInt_enable(CC_FIFO_PIN,SOS_FALLING_EDGE,fifo_interrupt);
}

void TC_Enable_Interrupt()
{
	PXA27XGPIOInt_enable(CC_FIFOP_PIN,SOS_RISING_EDGE,fifop_interrupt);
}

void TC_Disable_Interrupt()
{
	PXA27XGPIOInt_disable(CC_FIFOP_PIN);
}

void fifop_interrupt(void)
{
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	//ker_led(LED_GREEN_TOGGLE);

	if (!TC_FIFOP_IS_SET) {
		PXA27XGPIOInt_clear(CC_FIFOP_PIN);
		LEAVE_CRITICAL_SECTION();
		return;
	}

	while( TC_FIFO_IS_SET ) {
		if(_FIFOP_IS_SET_CALL)
//			_FIFOP_IS_SET_CALL(timestamp);
			_FIFOP_IS_SET_CALL(st_timestamp); //we want the timestamp when it receives the first byte
	}
	if (TC_FIFOP_IS_SET) {		// Detect rxfifo overflow
		TC_FLUSH_RX;
		TC_STROBE(CC2420_SNOP);
		LEAVE_CRITICAL_SECTION();
		return;
	}
	LEAVE_CRITICAL_SECTION();
}

void fifo_interrupt(void)
{
//showbyte(0x00);
	st_timestamp = getTime();
	if(_FIFO_IS_SET_CALL)
		_FIFO_IS_SET_CALL(st_timestamp);
}

/*****************************************************************
 * define these two functions for hardware.c                     *
 *****************************************************************/
void ker_radio_ack_enable()
{
	TC_ENABLE_ADDR_CHK;
}
void ker_radio_ack_disable()
{
	TC_DISABLE_ADDR_CHK;
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
