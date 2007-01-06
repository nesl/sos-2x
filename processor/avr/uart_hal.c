/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief    uart hdlc driver
 * @author	 Naim Busek <ndbusek@gmail.com>
 *
 */

#include <hardware.h>
#include <net_stack.h>
#include <sos_info.h>
#include <crc.h>
#include <measurement.h>
#include <malloc.h>

#include <uart_hal.h>

static bool uart_initialized = false;

int8_t uart_hardware_init(void){
	HAS_CRITICAL_SECTION;

	if(uart_initialized == false) {
		ENTER_CRITICAL_SECTION();
#ifndef DISABLE_UART
#ifndef USE_UART1
		//! UART will run at: 115kbps, N-8-1
		//! Set 115.2 KBps
		// Warning: 115.2 may cause problems with MIB510 type files
        //UBRR0H = (uint8_t) (BAUD_115_2k_U2X>>8);
		//UBRR0L = (uint8_t) (BAUD_115_2k_U2X);
		//! Set 57.6 KBps
		UBRR0H = (uint8_t) (BAUD_57_6k_U2X>>8);
		UBRR0L = (uint8_t) (BAUD_57_6k_U2X);

		//! Set UART double speed
		UCSR0A = (1 << U2X);
		//! Set frame format: 8 data-bits, 1 stop-bit
		UCSR0C = ((1 << UCSZ1) | (1 << UCSZ0));
		/**
		 * Enable reciever and transmitter and their interrupts
		 * transmit interrupt will be disabled until there is 
		 * packet to send.
		 */

		UCSR0B = ((1 << RXCIE) | (1 << RXEN) | (1 << TXEN));
#else

		UBRR1H = (uint8_t) (BAUD_57_6k_U2X>>8);
		UBRR1L = (uint8_t) (BAUD_57_6k_U2X);
		UCSR1A = (1 << U2X);
		UCSR1C = ((1 << UCSZ1) | (1 << UCSZ0));
		UCSR1B = ((1 << RXCIE) | (1 << RXEN) | (1 << TXEN));
#endif

#ifdef SOS_USE_PRINTF
		fdevopen(uart_putchar, NULL, 0);
#endif
#else
		uart_disable();
#endif

		LEAVE_CRITICAL_SECTION();
		uart_initialized = true;
	}
	return SOS_OK;
}

