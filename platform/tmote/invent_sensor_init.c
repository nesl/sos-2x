/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/* 
 * Authors: Akhilesh Singhania
 *
 */
#include <sos.h>
#include <msp430/usart.h>

#define NO_DEVICES 2
uint8_t devices[NO_DEVICES] = 
  { 0x2C, //! Microphone Gain Control
    0x2D  //! Microphone Compression Control
  };
static uint8_t counter = 0;

void invent_sensor_init( void ) 
{
  if (NO_DEVICES < 1) return;

  // I2C Module Initialization
  // setting USART to I2C
  U0CTL &= ~(I2CEN);
  U0CTL |= (SYNC | I2C);
  // Setting I2C data register
  // Setting to transmit mode and data length to word
  I2CTCTL &= ~(I2CWORD);
  I2CTCTL |= I2CTRX;
  // Set to Master Mode and in idle
  I2CTCTL &= ~(I2CRM | I2CSTP | I2CSTT);
  // Enable interrupts on ARDYIE
  I2CIE |= I2CIV_ARDY;

  // Sending to the first slave device 
  I2CDR = devices[counter++] | 0x80;
  I2CNDAT = 1;

  // Start the transmission
  I2CTCTL &= ~(I2CRM);
  I2CTCTL |= (I2CSTP | I2CSTT);
}

interrupt(USART0RX_VECTOR) i2c_interrupt() 
{
  if ((I2CIV & (1<<ARDYIFG)) == 1) {
    if(counter < NO_DEVICES) {
      I2CDR = devices[counter++] | 0x80;
      I2CNDAT = 1;
      // Start the transmission
      I2CTCTL &= ~(I2CRM);
      I2CTCTL |= (I2CSTP | I2CSTT);
    }
    else {
      // Disable and Reset I2C
      U0CTL &= ~(I2C | SYNC | I2CEN);
      U0CTL |= SWRST;
    }
  }
}
