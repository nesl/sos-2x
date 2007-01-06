/* ex: set ts=4: */
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

/**
 *	UART configuration
 *
 * 		UART will run at: N-8-1 and 57.6 KBps
 *
 *		1.	Initialize UART
 *		2.	configure gpio ports
 *		3.	configure uart control register
 *		4.	specify uart baud rate
 *		5.	configure uart transfer control
 *		6.	configure interrupt control
 *		7.	end of uart initialization
 *
 *	Pins
 *
 *		PIOA[0]		RX in
 *		PIOA[1]		TX out
 *
 *	Registers
 *
 *		UARTRBR		receiver buffer register
 *		UARTTHR		transmitter holding register
 *		UARTIER		interrupt enable register
 *		UARTIIR		interrupt identification register
 *		UARTFCR		fifo control register
 *		UARTLCR		line control register
 *		UARTMCR		modem control register
 *		UARTLSR		line status register
 *		UARTMSR		modem status register
 *		UARTSCR		scratch register
 *		UARTDLL		divisor latch (LSB)
 *		UARTDLM		divisor latch (MSB)
 *
 * 	UART interrupt initialization
 *
 * 		Enable reciever and transmitter and their interrupts
 * 		transmit interrupt will be disabled until there is
 * 		packet to send.
 *
 *	fdevopen(uart_putchar, NULL, 0);
 *
 */

/**
 * @brief UART driver for XYZ platform
 * @author Andrew Barton-Sweeney (abs@cs.yale.edu)
 *
 */


#include <sos.h>

#if (defined USE_UART_STREAM || defined USE_UART_RAW_STREAM || defined SOS_USE_PRINTF)
#include <uart_stream.h>
#else
#include <sos_uart.h>
#endif

#include "uart_hal.h"

void uart_setByte(char b) 	{ put_value(UARTTHR,b); }
char uart_getByte() 		{ return get_value(UARTRBR); }
char uart_checkError() 		{ return (get_value(UARTLSR) & 0x1E); }
void uart_enable_tx() 	{ put_value(UARTIER,3); }
void uart_disable_tx() 	{ put_value(UARTIER,1); }

#define UART_EN			0x0040
#define UART_FORCEOFF	0x0080

/**
 * uart_interrupt_handler()
 */
static void uart_interrupt_handler()
{
	char stat = get_value(UARTIIR);

	switch (stat & 0x0E) {
		case (0x04) : 				/* data ready */
		case (0x0C) :				/* character timeout */
		case (0x06) : {				/* line status */
			uart_recv_int_();
			break;
		}
		case (0x02) : { 			/* tx empty */
			uart_send_int_();
			break;
		}
	}
}

/**
 * uart_hardware_init()
 */
static bool uart_initialized = false;
void uart_hardware_init(void)
{
    //HAS_CRITICAL_SECTION;
    //ENTER_CRITICAL_SECTION();
    //LEAVE_CRITICAL_SECTION();

	if(uart_initialized == false) {
    set_hbit(GPCTL,GPCTL_UART);
    put_hvalue(GPPMB,0);		// disable SIO bits 6 and 7
   	put_hvalue(GPPOB,0);
    put_hvalue(GPPMA,0x0002);	// RX is input (A0), TX is output (A1)
   	put_hvalue(GPPOA,0);

    put_value(UARTIER,0);		// no interrupts
    put_value(UARTLCR,0x80);	// enable access to divisor latch
    put_value(UARTDLM,0);
    put_value(UARTDLL,0x3E);	// 57600 baud
    put_value(UARTLCR,0x03);
    put_value(UARTFCR,0xC7);	// 14 byte trigger, cleared and buffered (auto set to normal FCR mode)
    put_value(UARTMCR,0);
    put_value(UARTIER,0x01);	// RX

    IRQ_HANDLER_TABLE[INT_UART]=uart_interrupt_handler;
	set_wbit(ILC1,ILC1_ILR9 & ILC1_INT_LV3);

	set_hbit(GPPMC, UART_EN); 									// set PIOC6(UART_EN) to output
	set_hbit(GPPMC, UART_FORCEOFF); 									// set PIOC7(FORCEOFF) to output

    set_hbit(GPCTL,GPCTL_UART);
    put_hvalue(GPPMA,0x0002);	// RX is input (A0), TX is output (A1)
   	put_hvalue(GPPOA,0);
	clr_hbit(GPPOC,UART_EN);
	set_hbit(GPPOC,UART_FORCEOFF);	// this also drives FORCEON -- they are tied together

	uart_initialized = true;
	}
}
