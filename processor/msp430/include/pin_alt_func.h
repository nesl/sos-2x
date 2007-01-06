/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/**
 * @file pin_alt_func.h
 * @brief Alternate Pin Functions for MSP430F1611
 * @author Naim Busek
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#include <pin_defs.h>

#ifndef _PIN_ALT_FUNC_H_
#define _PIN_ALT_FUNC_H_
		
//----------------------------------------------------------------------
// PORT 1
//----------------------------------------------------------------------
ALIAS_IO_PIN( TACLK, P1_0);
// ALIAS_IO_PIN( TA0, P1_1);
ALIAS_IO_PIN( CCI0A, P1_1);
// ALIAS_IO_PIN( TA1, P1_2);
ALIAS_IO_PIN( CCI1A, P1_2);
// ALIAS_IO_PIN( TA2, P1_3);
ALIAS_IO_PIN( CCI2A, P1_3);
//ALIAS_IO_PIN( SMCLK, P1_4);
ALIAS_IO_PIN( TA0, P1_5);
ALIAS_IO_PIN( TA1, P1_6);
ALIAS_IO_PIN( TA2, P1_7);
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// PORT 2
//----------------------------------------------------------------------
//ALIAS_IO_PIN( ACLK, P2_0);
ALIAS_IO_PIN( TAINCLK, P2_1);
ALIAS_IO_PIN( CAOUT, P2_2);
ALIAS_IO_PIN( CA0, P2_3);
// ALIAS_IO_PIN( TA1, P2_3);
ALIAS_IO_PIN( CA1, P2_4);
// ALIAS_IO_PIN( TA2, P2_4);
ALIAS_IO_PIN( Rosc, P2_5);
ALIAS_IO_PIN( ADC12CLK, P2_6);
ALIAS_IO_PIN( DMAE0, P2_6);
// ALIAS_IO_PIN( TA0, P2_7);
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// PORT 3
//----------------------------------------------------------------------
ALIAS_IO_PIN(  STE0, P3_0); // Slave Transmit Enable - USART0/SPI Mode
ALIAS_IO_PIN( SIMO0, P3_1); // Slave In Master Out   - USART0/SPI Mode
ALIAS_IO_PIN(   SDA, P3_1); // I2C Data              - USART0/I2C Mode
ALIAS_IO_PIN( SOMI0, P3_2); // Slave Out Master In   - USART0/SPI Mode
ALIAS_IO_PIN( UCLK0, P3_3); // SPI Clock IO          - USART0/SPI Mode
ALIAS_IO_PIN(   SCL, P3_3); // I2C Clock             - USART0/I2C Mode
ALIAS_IO_PIN( UTXD0, P3_4); // Transmit Data Out     - USART0/UART Mode
ALIAS_IO_PIN( URXD0, P3_5); // Receive Data In       - USART0/UART Mode
ALIAS_IO_PIN( UTXD1, P3_6); // Transmit Data Out     - USART1/UART Mode
ALIAS_IO_PIN( URXD1, P3_7); // Receive Data In       - USART1/UART Mode
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// PORT 4
//----------------------------------------------------------------------
ALIAS_IO_PIN( TB0, P4_0);
ALIAS_IO_PIN( TB1, P4_1);
ALIAS_IO_PIN( TB2, P4_2);
ALIAS_IO_PIN( TB3, P4_3);
ALIAS_IO_PIN( TB4, P4_4);
ALIAS_IO_PIN( TB5, P4_5);
ALIAS_IO_PIN( TB6, P4_6);
ALIAS_IO_PIN( TBCLK, P4_7);
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// PORT 5
//----------------------------------------------------------------------
ALIAS_IO_PIN( STE1, P5_0);
ALIAS_IO_PIN( SIMO1, P5_1);
ALIAS_IO_PIN( SOMI1, P5_2);
ALIAS_IO_PIN( UCLK1, P5_3);
ALIAS_IO_PIN( MCLK, P5_4);
ALIAS_IO_PIN( SMCLK, P5_5);
ALIAS_IO_PIN( ACLK, P5_6);
ALIAS_IO_PIN( TBOUTH, P5_7);
ALIAS_IO_PIN( SVSOUT, P5_7);
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// PORT 6
//----------------------------------------------------------------------
ALIAS_IO_PIN( A0, P6_0);
ALIAS_IO_PIN( A1, P6_1);
ALIAS_IO_PIN( A2, P6_2);
ALIAS_IO_PIN( A3, P6_3);
ALIAS_IO_PIN( A4, P6_4);
ALIAS_IO_PIN( A5, P6_5);
ALIAS_IO_PIN( A6, P6_6);
ALIAS_IO_PIN( DAC0, P6_6);
ALIAS_IO_PIN( A7, P6_7);
ALIAS_IO_PIN( DAC1, P6_7);
ALIAS_IO_PIN( SVSIN, P6_7);
//----------------------------------------------------------------------

#endif // _PIN_ALT_FUNC_H_
