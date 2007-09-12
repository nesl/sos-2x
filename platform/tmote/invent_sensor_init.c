/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/* 
 * Authors: Akhilesh Singhania
 *
 */
#include <sos.h>
#include <msp430/usart.h>

#define LED_DEBUG
#include <led_dbg.h>

#define NO_DEVICES 2
uint8_t devices[NO_DEVICES] = 
  { 0x2C, //! Microphone Gain Control
    0x2D  //! Microphone Compression Control
  };
uint8_t data[NO_DEVICES] = 
  { 0x01, //! Microphone Gain Control
    0x02  //! Microphone Compression Control
  };
static uint8_t counter = 0;
//static uint8_t wait_flag = 1;

void invent_sensor_init( void ) 
{
  if (NO_DEVICES < 1) return;

	LED_DBG(LED_RED_OFF);
	LED_DBG(LED_GREEN_OFF);
	LED_DBG(LED_YELLOW_OFF);
	LED_DBG(LED_YELLOW_ON);
	
	// Disbale UART and SPI modules
	U0ME &= ~(UTXE0 | URXE0 | USPIE0);
	P3SEL &= ~(BV(1) | BV(2) | BV(3) | BV(4) | BV(5));

	// Select input direction for SIMO and UCLK
	P3DIR &= ~(BV(1) | BV(3));
	// Select module function for SIMO and UCLK
	P3SEL |= (BV(1) | BV(3));

	// Disable interrupts
	U0IE &= ~(UTXIE0 | URXIE0);

	// Power ON the tmote invent sensor board
	// Select I/O function for UART0RX (POT_SHDN) pin.
	P3SEL &= ~BV(5);
	// Set direction as output
	P3DIR |= BV(5);
	// Set output to LOW to turn ON the sensor board.
	P3OUT &= ~BV(5);

	// I2C Module Initialization
	// Enable the I2C module using SWRST
	U0CTL |= SWRST;
	// Select I2C operation.
	U0CTL |= (SYNC | I2C);
	// Disbale I2C module before configuring it.
	U0CTL &= ~(I2CEN);
	// Select Master mode.
	U0CTL |= MST;
	// Select clock source (SMCLK).
	I2CTCTL |= I2CSSEL_2;
	// Setting data length to byte
	// I2CNDAT controls number of bytes transmitted.
	I2CTCTL &= ~(I2CWORD | I2CRM);
	// Select clock generation parameters
	I2CPSC = 0x00;
	I2CSCLH = 0x03;
	I2CSCLL = 0x03;
	// Set own address
	I2COA |= 0x01;
	// Configuration over.
	// Enable the I2C module again.
	U0CTL |= I2CEN;
	// Set transmit mode
	// I2CSTB: Send START byte when I2CSTT is set
	I2CTCTL |= (I2CTRX | I2CSTB);
	// Set master in idle mode
	I2CTCTL &= ~(I2CSTP | I2CSTT);
	// Enable interrupts on ARDYIE
	I2CIE = ARDYIE;
	I2CIE = 0;
	I2CIFG = 0;
	//U0IE |= (URXIE0);
	//U0IE &= ~(UTXIE0);

	// Sending to the first slave device 
	I2CSA = devices[counter];
	I2CNDAT = 1;

	// Start the transmission in master mode 2
	I2CTCTL |= (I2CSTP | I2CSTT);
	I2CDR = data[counter++];

	// Wait till complete sensor board has
	// been initialized.
	while (I2CDCTL & I2CBUSY);
	LED_DBG(LED_GREEN_ON);
	//while (wait_flag);

	// Select Master Tx mode.
	U0CTL |= MST;
	I2CTCTL |= (I2CTRX | I2CSTB);
	I2CSA = devices[counter];
	I2CNDAT = 1;
	// Start the transmission
	I2CTCTL |= (I2CSTP | I2CSTT);
	I2CDR = data[counter++];

	while (I2CDCTL & I2CBUSY);
	
	LED_DBG(LED_RED_ON);

	// Disable and Reset I2C
	U0CTL &= ~(I2C | SYNC | I2CEN);
	U0CTL |= SWRST;
}

/*
interrupt (USART0RX_VECTOR) i2c_interrupt() 
{
	LED_DBG(LED_GREEN_TOGGLE);
	if ((I2CIFG & ARDYIFG) == 1) {
		if (counter < NO_DEVICES) {
			I2CSA = devices[counter];
			I2CNDAT = 1;
			// Select Master mode.
			U0CTL |= MST;
			// Start the transmission
			I2CTCTL |= (I2CSTP | I2CSTT);
			I2CDR = data[counter++];
		}
		else {
			// Disable and Reset I2C
			U0CTL &= ~(I2C | SYNC | I2CEN);
			U0CTL |= SWRST;
			wait_flag = 0;
		}
  }
}
*/

