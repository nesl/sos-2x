/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
      
#ifndef _ADS8341_ADC_H_
#define _ADS8341_ADC_H_

#include "ads8341.h"

#include <adc_cb.h>

#define ADS8341_ADC_WR_LEN 1
#define ADS8341_ADC_RD_LEN 3

#define ADS8341_ADC_PORTMAPSIZE 4

typedef struct bind_msg {
	uint8_t port;
	uint8_t adcPort;
} bind_msg_t;

typedef struct set_gain_msg {
	uint8_t gain;
} set_gain_msg_t;

typedef struct get_data_msg {
	uint8_t port;
	uint8_t flags;
} get_data_msg_t;

enum {
	ADS8341_ADC_PORT0=0,
	ADS8341_ADC_PORT1,
	ADS8341_ADC_PORT2,
	ADS8341_ADC_PORT3,
};

/* 7.3728 MHz/CLK_XXX = X.X KHz (prescaler) */
/* X.X KHz x interval/255 = rate Hz */
/* interval = rate*255/X.X KHz */

enum {
	ADS8341_CLK_OFF=0,  //			0 Hz
	ADS8341_CLK_1,			// 7.3728 MHz
	ADS8341_CLK_8,			//  921.6 KHz
	ADS8341_CLK_64,			//  115.2 KHz
	ADS8341_CLK_256,		//	 28.8 KHz
	ADS8341_CLK_1024,		//		7.2 KHz
	ADS8341_CLK2_TRAILING,
	ADS8341_CLK2_LEADING,
};

/** msg types for config messages */
enum {
	ADS8341_BIND_PORT=0,
	ADS8341_SET_GAIN,
};

enum {
	ADS8341_FID0=0,
	ADS8341_FID1,
	ADS8341_FID2,
	ADS8341_FID3,
};
	
extern int8_t ads8341_adc_init();
extern int8_t ads8341_adc_hardware_init();

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_ads8341_adc_bindPort(uint8_t port, uint8_t adcPort, uint8_t calling_id, adc16_cb_t func);
extern int8_t ker_ads8341_adc_getData(uint8_t port);
// these may be able to be meared into a single call
extern int8_t ker_ads8341_adc_getPerodicData(uint8_t port, uint8_t prescaler, uint8_t interval, uint16_t count);
extern int8_t ker_ads8341_adc_getDataDMA(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t *sharedMemory, uint16_t sampleCnt);

extern int8_t ker_ads8341_adc_stopPerodicData(uint8_t port);
#endif

#endif // _ADS8341_ADC_H_

