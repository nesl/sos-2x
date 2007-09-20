/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/* 
 * Authors: Akhilesh Singhania
 * 			Rahul Balani
 *
 */
#include <sos.h>
#include <msp430/usart.h>

#define LED_DEBUG
#include <led_dbg.h>

// Number of devices that need to be initialized
// over I2C.
#define NUM_DEVICES 3

uint8_t devices[NUM_DEVICES] = 
{ 0x2F, //! Accelerometer/Photo Control
  0x2C, //! Microphone Gain Control
  0x2D, //! Microphone Compression Control
};
uint8_t length[NUM_DEVICES] = 
{ 0x02, //! Accelerometer/Photo Control
  0x02, //! Microphone Gain Control
  0x02, //! Microphone Compression Control
};
uint8_t photo_data[2] =
{ 0x18,	//! Turn ON Accelerometer and Photo sensor
  0x00,	//! Dummy data
};
//! Microphone variable gain config written to RDAC 0 of AD5242 Pot
uint8_t mic_gain_data[2] =
{ 0x10,	//! Turn ON microphone 
  0x1A,	//! Microphone default gain (530 ohms, -44 dBV noise gate)
};
//! Microphone variable compression config written to RDAC 0 of AD5242 Pot
uint8_t mic_compression_data[2] =
{ 0x00,	//! Keep speaker OFF, select RDAC 0 
  0x04,	//! Microphone default compression ratio (2:1, 17 kohms)
};
uint8_t *data[NUM_DEVICES] = 
{ photo_data,
  mic_gain_data,
  mic_compression_data,
};
static uint8_t counter = 0;
static uint8_t ptr = 0;

void invent_sensor_init( void ) 
{
	HAS_CRITICAL_SECTION;

	// Save previous interrupt enable flag.
	uint8_t ie2 = IE2;

	if (NUM_DEVICES < 1) return;

	// Disable other interrupts (UART1) before enabling global interrupts
	// for I2C.
	IE2 &= ~(URXIE1 | UTXIE1); 
	// Clear interrupt flags.
	IFG1 = 0;
	IFG2 = 0;
	// Enable global interrupts so that we can use I2C interrupts
	// for data transfer
	ENABLE_GLOBAL_INTERRUPTS();

	ENTER_CRITICAL_SECTION();

	LED_DBG(LED_RED_OFF);
	LED_DBG(LED_GREEN_OFF);
	LED_DBG(LED_YELLOW_OFF);

	// Diable I2C module before initialization.
	// Just for safety.
	// Then, follow the exact procedure as listed here,
	// otherwise unpredictable behaviour will occur.
	U0CTL &= ~(I2C | SYNC | I2CEN);

	// Power ON the tmote invent sensor board
	// Select I/O function for UART0RX (POT_SHDN) pin.
	P3SEL &= ~BV(5);
	// Set direction as output
	P3DIR |= BV(5);
	// Set output to HIGH to turn ON the sensor board.
	P3OUT |= BV(5);

	// I2C Module Initialization
	// Enable the I2C module using SWRST
	U0CTL = SWRST;
	// Select I2C operation.
	U0CTL |= (SYNC | I2C);
	// Disbale I2C module before configuring it.
	U0CTL &= ~(I2CEN);

	// Disbale UART and SPI modules
	U0ME &= ~(UTXE0 | URXE0 | USPIE0);
	P3SEL &= ~(BV(1) | BV(2) | BV(3) | BV(4) | BV(5));

	// Select input direction for SIMO and UCLK
	P3DIR &= ~(BV(1) | BV(3));
	// Select module function for SIMO and UCLK
	P3SEL |= (BV(1) | BV(3));

	// Disable UART0 and SPI interrupts
	IE1 &= ~(UTXIE0 | URXIE0);

	// Select Master mode.
	U0CTL |= MST;
	// Select clock source (SMCLK = 1 Mhz), I2C operates at 1/10 Mhz = 100 Khz.
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
	LEAVE_CRITICAL_SECTION();

	while (counter < NUM_DEVICES) {
		ENTER_CRITICAL_SECTION();

		// Reset the ptr to data bytes.
		ptr = 0;
		// Disable I2C before re-configuring it.
		U0CTL &= ~I2CEN;
		// Select Master mode.
		U0CTL |= MST;
		// Set slave device address 
		I2CSA = devices[counter];
		// Set number of bytes to be transmitted
		I2CNDAT = length[counter];
		// Enable I2C again.
		U0CTL |= I2CEN;
		// Set Tx mode.
		I2CTCTL |= I2CTRX;
		// Enable Tx ready and No Ack interrupts
		I2CIE = TXRDYIE | NACKIE;
		I2CIFG = 0;
		// Start the transmission
		I2CTCTL |= (I2CSTP | I2CSTT);

		LEAVE_CRITICAL_SECTION();

		// Wait till data transmission is complete.
		while (I2CDCTL & I2CBUSY);

		// Increment counter to configure next device
		counter++;
	}

	DISABLE_GLOBAL_INTERRUPTS();

	LED_DBG(LED_YELLOW_ON);

	// Reset IE2 mask
	IE2 = ie2;
	IFG2 = 0;

	// Disable and Reset I2C
	U0CTL &= ~(I2C | SYNC | I2CEN);
	U0CTL |= SWRST;

}

interrupt (USART0TX_VECTOR) i2c_tx_interrupt() 
{
	volatile uint16_t _value = I2CIV;
	switch (_value) {
		case 0x000C: {
			// Ready to load I2CDR
			// Clear the TXRDY interrupt flag
			I2CIFG &= ~TXRDYIFG;
			// Load the I2CDR with next byte.
			I2CDR = data[counter][ptr++];
			if (ptr == length[counter]) {
				// This is the last byte for current device.
				I2CIE &= ~TXRDYIE;
			}
			break;
		}
		case 0x0004: {
			// No ACK from slave
			// Disable the I2C to clear I2CDTCL flags
			U0CTL &= ~I2CEN;
			// Clear the NACK interrupt flag
			I2CIFG &= ~NACKIFG;
			break;
		}
		default: break;
	}
}

