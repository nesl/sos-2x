/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief This module uses the msp430 DMA to transfer data from ADC12 port 0 to a memory location.
 *
 * Use adc_start to initiate a adc/dma transfer. Once the transfer 
 * is done, you will receive a ADC_DATA_READY_MSG.
 */


#ifndef __ADC_H__
#define __ADC_H__
// we source SMCLK which is set to 1MHz in SOS
#define CPU_CLOCK 1000000

enum adcRate {ONE_KHZ = ((CPU_CLOCK)/1000), 
              TWO_KHZ =((CPU_CLOCK)/2000), 
              FOUR_KHZ =((CPU_CLOCK)/4000), 
              EIGHT_KHZ=((CPU_CLOCK)/8000)};


#define ADC_DATA_READY_MSG 153

/*
Starts an ADC transefer and writes to the memory reagion 
specified by start address and length (results are unsigned ints)
returns SOS_OK if ADC was started successfully.
*/
int8_t adc_start(uint8_t spid, uint16_t* , uint16_t, enum adcRate);

/*
stop the ADC. Should be called after the DMA transfer ends
*/
void adc_stop( );

#endif
