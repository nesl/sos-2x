/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @file spi_hal.c
 * @brief SPI HAL for MSP430F1611 micro-controller
 * @author Ram Kumar {ram@ee.ucla.edu}
 */
//------------------------------------------------------------------------
// INCLUDES
//------------------------------------------------------------------------
#include <sos.h>
#include <hardware.h>
#include <spi_hal.h>
#include <pin_alt_func.h>

//------------------------------------------------------------------------
// LOCAL GLOBALS
//------------------------------------------------------------------------
static uint8_t spi_init;

//------------------------------------------------------------------------
// SPI HARDWARE INIT
//------------------------------------------------------------------------
void spi_hardware_init() 
{
	HAS_CRITICAL_SECTION;
	
	if (spi_init == 0){
		ENTER_CRITICAL_SECTION();
		{
			spi_init = 1;
			// Set the PORT3 Pin Functions and Directions
			// PORT3 - SPI Peripheral Moudle Functions
			// P3.0: SLAVE TRANSMIT ENABLE
			// P3.1: SLAVE IN MASTER OUT
			// P3.2: SLAVE OUT MASTER IN
			// P3.3: CLOCK INPUT/OUTPUT
			STE0_UNSET_ALT();   // Using 3-wire SPI
			SIMO0_SET_ALT();    
			SOMI0_SET_ALT();
			UCLK0_SET_ALT();
			
			SET_STE0_DD_IN();
			SET_SIMO0_DD_OUT();
			SET_SOMI0_DD_IN();
			SET_UCLK0_DD_OUT();
			
			// USART0 UART module disable
			ME1 &= ~(UTXE0 | URXE0);        
			UTXD0_UNSET_ALT(); 
			URXD0_UNSET_ALT();
			SET_UTXD0_DD_IN();
			SET_URXD0_DD_IN();
			
			// USART0 I2C module disable
			U0CTL &= ~(I2C | I2CEN | SYNC);    
			
			U0CTL = SWRST;              // SW Reset of USART0
			U0CTL |= CHAR | SYNC | MM;  // 8-bit char, SPI-mode, USART as master
			U0CTL &= ~I2C;              // Disable I2C, Enable SPI
			
			U0TCTL = STC ;        // 3-pin mode, STE disabled
			U0TCTL |= CKPH;       // half-cycle delayed UCLK
			U0TCTL |= SSEL_SMCLK; // use SMCLK, assuming 1MHz
			
			// SPI Master Mode Baud Rate
			// Baud Rate = (BRCLK)/[U0BR1, U0BR0]
			U0BR0 = 0x02;         // Fastest Baud-rate
			U0BR1 = 0x00;         // Baud Rate = (BRCLKD/2); 
			
			U0MCTL = 0;           // Modulation should be set to 0x00 for SPI mode
			
			ME1 |= USPIE0;        // USART SPI module enable
			U0CTL &= ~SWRST;      // SW Reset Cleared, SPI Ready
			
			IE1 |= (UTXIE0 | URXIE0); // Enable Transmit and Receive Interrupts
		}    
		LEAVE_CRITICAL_SECTION();
	}
	return;
}

//------------------------------------------------------------------------
// TX DATA
//------------------------------------------------------------------------
inline void spiTx(uint8_t data) {
   U0TXBUF = data;
}

//------------------------------------------------------------------------
// RX DATA
//------------------------------------------------------------------------
inline uint8_t spiRx() {
   return U0RXBUF;
}

//------------------------------------------------------------------------
// CHECK PENDING TX INTERRUPT
//------------------------------------------------------------------------
inline bool spiIsTxIntrPending() {
   if (IFG1 & UTXIFG0){
      IFG1 &= ~UTXIFG0;
      return true;
   }
   return false;
}

//------------------------------------------------------------------------
// CHECK PENDING RX INTERRUPT
//------------------------------------------------------------------------
inline bool spiIsRxIntrPending() {
   if (IFG1 & URXIFG0){
      IFG1 &= ~URXIFG0;
      return true;
   }
   return false;
}

//------------------------------------------------------------------------
// CHECK EMPTY TX BUFFER
//------------------------------------------------------------------------
inline bool spiIsTxEmpty() {
   if (U0TCTL & TXEPT) {
     return true;
   }
   return false;
} 

//------------------------------------------------------------------------
// DISABLE RX INTERRUPT
//------------------------------------------------------------------------
inline void spiDisableRxIntr(){
   HAS_CRITICAL_SECTION;
   ENTER_CRITICAL_SECTION();
   IE1 &= ~URXIE0;    
   LEAVE_CRITICAL_SECTION();
}

//------------------------------------------------------------------------
// DISABLE TX INTERRUPT
//------------------------------------------------------------------------
inline void spiDisableTxIntr(){
   HAS_CRITICAL_SECTION;
   ENTER_CRITICAL_SECTION();
   IE1 &= ~UTXIE0;    
   LEAVE_CRITICAL_SECTION();
}

