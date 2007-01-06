/*
 * Copyright (c) 2005 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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
/*
 *  @brief  UART driver for the iMote2. This calls into the SOS UART system.
 *  @author Andrew Barton-Sweeney   (abs@cs.yale.edu)
 */

#include <hardware.h>
#include <net_stack.h>
#include <sos_info.h>
#include <crc.h>
#include <measurement.h>
#include <malloc.h>
#include <uart_hal.h>
#include "uart.h"
#include "hardware.h"
#include <irq.h>

void HPLSTUART_Interrupt_fired(void);

uint32_t baudrate = 115200;

static bool uart_initialized = false;

int8_t stuart_init();
int8_t uart_setRate(uint32_t baudrate);
int8_t uart_stop();

int8_t uart_hardware_init(void){
	HAS_CRITICAL_SECTION;

	if(uart_initialized == false) {
		ENTER_CRITICAL_SECTION();
#ifndef DISABLE_UART
		/**
		 * Setup UART: 115kbps, N-8-1
		 *
		 * Enable reciever and transmitter and their interrupts
		 * transmit interrupt will be disabled until there is
		 * packet to send.
		 */
		stuart_init();
# ifdef SOS_USE_PRINTF
		fdevopen(uart_putchar, NULL, 0);
# endif
#else
		uart_disable();
#endif
		LEAVE_CRITICAL_SECTION();
		uart_initialized = true;
	}
	return SOS_OK;
}

int8_t stuart_init()
{
	/***
	need to configure the ST UART pins for the correct functionality

	GPIO<46> = STDRXD = ALT2(in)
	GPIO<47> = STDTXD = ALT1(out)
	*********/
	//configure the GPIO Alt functions and directions
	_GPIO_setaltfn(46,2);
	_GPIO_setaltfn(47,1);

	_GPDR(46) &= ~_GPIO_bit(46);
	_GPDR(47) |= _GPIO_bit(47);

	STLCR |=LCR_DLAB; //turn on DLAB so we can change the divisor
	STDLL = 8;  //configure to 115200;
	STDLH = 0;
	STLCR &= ~(LCR_DLAB);  //turn off DLAB

	STLCR |= 0x3; //configure to 8 bits

	STMCR &= ~MCR_LOOP;
	STMCR |= MCR_OUT2;
	STIER |= IER_RAVIE;
	//STIER |= IER_TIE;
	STIER |= IER_UUE; //enable the UART

	//STMCR |= MCR_AFE; //Auto flow control enabled;
	//STMCR |= MCR_RTS;

	STFCR = FCR_TRFIFOE; //enable the fifos

	PXA27XIrq_allocate(PPID_STUART, HPLSTUART_Interrupt_fired);
	PXA27XIrq_enable(PPID_STUART);

	CKEN |= CKEN5_STUART; //enable the UART's clk
	return 0;
}

int uart_is_disabled()
{
	return (STIER | IER_TIE);
}

void uart_enable_tx()
{
	STIER |= IER_TIE;
	//uint8_t intSource = STIIR;	// read IIR to clear transmit interrupt
	//intSource = intSource + 1;
}

void uart_disable_tx()
{
	STIER &= ~IER_TIE;
}

int8_t uart_setRate(uint32_t newbaudrate)
{
	return 0;
}

int8_t uart_stop()
{
	CKEN &= ~CKEN5_STUART;
	return 0;
}

void uart_setByte(char b)
{
	STTHR = b;
}

char uart_getByte()
{
	return STRBR;
}

char uart_checkError()
{
	return STLSR & LSR_FIFOE;		// should we need to return the cached error instead?
}

void HPLSTUART_Interrupt_fired()
{
	uint8_t error,intSource = STIIR;
	intSource = (intSource >> 1) & 0x3;
	switch (intSource)
	{
		case 0:			// Modem Status
			break;
		case 1:			// Ready to send
			uart_send_int_();
			break;
		case 2:			// Received Data Available
			while(STLSR & LSR_DR){
				uart_recv_int_();	// should we spin?
			}
			break;
		case 3:			// Receive Error
			error = STLSR;
			break;
	}
	return;
}

