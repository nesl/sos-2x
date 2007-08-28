#ifndef _ADC_DRIVER_COMMON_H_
#define _ADC_DRIVER_COMMON_H_

// ADC definitions commonly used by ADC driver and sensor drivers
// implemented as either kernel extensions or loadable modules.

#include <msp430x16x.h>
#include <sensor_system.h>

/**
 * 8 external + 4 internal ADC channel bit-mask
 * Please update the definitions below to add more
 * ADC channels.
 */
#define ADC_DRIVER_CH0 	0x0001L
#define ADC_DRIVER_CH1 	0x0002L
#define ADC_DRIVER_CH2 	0x0004L
#define ADC_DRIVER_CH3 	0x0008L
#define ADC_DRIVER_CH4 	0x0010L
#define ADC_DRIVER_CH5 	0x0020L
#define ADC_DRIVER_CH6 	0x0040L
#define ADC_DRIVER_CH7 	0x0080L
#define ADC_DRIVER_CH8 	0x0100L
#define ADC_DRIVER_CH9 	0x0200L
#define ADC_DRIVER_CH10	0x0400L
#define ADC_DRIVER_CH11	0x0800L
#define ADC_DRIVER_CHANNEL_BITMASK	\
			(ADC_DRIVER_CH0 | ADC_DRIVER_CH1 | \
			 ADC_DRIVER_CH2 | ADC_DRIVER_CH3 | \
			 ADC_DRIVER_CH4 | ADC_DRIVER_CH5 | \
			 ADC_DRIVER_CH6 | ADC_DRIVER_CH7 | \
			 ADC_DRIVER_CH8 | ADC_DRIVER_CH9 | \
			 ADC_DRIVER_CH10 | ADC_DRIVER_CH11 )

#define ADC_DRIVER_CHANNEL_MAPSIZE	12

#define ADC_DRIVER_CH_NULL 0xFFFFL

#define ADC_DRIVER_RESOLUTION	12

#if 0
// Settings for the ADC channels :
// ADC12 clock prescalers ( = x); 
// ADC_CLOCK_SCALE_x
#define ADC_CLOCK_SCALE_1	ADC12DIV_0
#define ADC_CLOCK_SCALE_2	ADC12DIV_1
#define ADC_CLOCK_SCALE_3	ADC12DIV_2
#define ADC_CLOCK_SCALE_4	ADC12DIV_3
#define ADC_CLOCK_SCALE_5	ADC12DIV_4
#define ADC_CLOCK_SCALE_6	ADC12DIV_5
#define ADC_CLOCK_SCALE_7	ADC12DIV_6
#define ADC_CLOCK_SCALE_8	ADC12DIV_7
#endif

// Internal reference voltage;
#define ADC_INTERNAL_REF2_5V	REF2_5V
#define ADC_INTERNAL_REF1_5V	0x00
// Sample time definition in terms of
// number of ADC12 clock cycles ( = y); 
// ADC_SAMPLE_TIME_y
// t_sample = y micro-seconds; @ 1 Mhz clock
#define ADC_SAMPLE_TIME_4 		SHT0_0
#define ADC_SAMPLE_TIME_8 		SHT0_1
#define ADC_SAMPLE_TIME_16 		SHT0_2
#define ADC_SAMPLE_TIME_32		SHT0_3
#define ADC_SAMPLE_TIME_64 		SHT0_4
#define ADC_SAMPLE_TIME_96 		SHT0_5
#define ADC_SAMPLE_TIME_128		SHT0_6
#define ADC_SAMPLE_TIME_192 	SHT0_7
#define ADC_SAMPLE_TIME_256 	SHT0_8
#define ADC_SAMPLE_TIME_384 	SHT0_9
#define ADC_SAMPLE_TIME_512 	SHT0_10
#define ADC_SAMPLE_TIME_768 	SHT0_11
#define ADC_SAMPLE_TIME_1024 	SHT0_12

// Command sent by sensor driver to ADC driver.
enum {
	ADC_REGISTER_REQUEST,
	ADC_REMOVE_REQUEST,
	ADC_GET_DATA,
};

// Feedback from ADC driver to sensor driver.
typedef enum {
	ADC_SENSOR_SEND_DATA	= 0,
	ADC_SENSOR_CHANNEL_UNBOUND,
	ADC_SENSOR_SAMPLING_DONE,
	ADC_SENSOR_ERROR,
} adc_feedback_t;

// Sensor specific configuration stored for each
// registered sensor driver.
typedef struct {
	uint16_t sht0;
	uint16_t ref2_5;
} sensor_config_t;


#endif

