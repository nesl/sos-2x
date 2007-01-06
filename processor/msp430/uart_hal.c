/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sos.h>

#include "uart_hal.h"

static bool uart_initialized = false;
void uart_hardware_init(void){
	HAS_CRITICAL_SECTION;

	if(uart_initialized == false) {
		ENTER_CRITICAL_SECTION();
		//! Configure hardware UART
		//! UART will run at: N-8-1
		//! Set 57.6 KBps
		// Set peripheral module function for UTXD1 and URXD1
		P3SEL |= (3 << 6); 

	  // Initialize the UART registers
	  U1CTL = SWRST;  
	  U1CTL |= CHAR;  // 8-bit char, UART-mode

	  U1RCTL &= ~URXEIE;  // even erroneous characters trigger interrupts

	  U1CTL = SWRST;
	  U1CTL |= CHAR;  // 8-bit char, UART-mode

	  U1TCTL &= ~(SSEL_0 | SSEL_1 | SSEL_2 | SSEL_3);
	  U1TCTL |= SSEL_SMCLK;
	  //SSEL_ACLK;  // clock source ACLK
	  //SSEL_SMCLK; // clock source SMCLK

    U1MCTL = SMCLK_BAUD_57_6k_UMCTL;
    U1BR1 = SMCLK_BAUD_57_6k_UBR1;
    U1BR0 = SMCLK_BAUD_57_6k_UBR0;
    
    ME2 &= ~USPIE1;   // USART1 SPI module disable

    ME2 |= (UTXE1 | URXE1);   // USART1 UART module enable
    
    U1CTL &= ~SWRST;
    
    IFG2 &= ~(UTXIFG1 | URXIFG1);
    // enable receive interrupt, transmit disabled until we have data to send
    IE2 |= URXIE1;
    IE2 &= ~(UTXIE1);
    
    LEAVE_CRITICAL_SECTION();

    uart_initialized = true;
  }
}

