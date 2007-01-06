/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
      
#ifndef _ADS8341_FUNC_ADC_H_
#define _ADS8341_FUNC_ADC_H_

#include <sos.h>
#include <adc_cb.h>

typedef int8_t (*ker_ads8341_adc_bindPort_func_t)(uint8_t port, uint8_t adcPort, uint8_t calling_id, adc16_cb_t func);
typedef int8_t (*ker_ads8341_adc_getData_func_t)(uint8_t port);
typedef int8_t (*ker_ads8341_adc_getCalData_func_t)(uint8_t port);
typedef int8_t (*ker_ads8341_adc_getDataDMA_func_t)(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t *sharedMemory, uint8_t sampleCnt);
// these may be able to be meared into a single call
typedef int8_t (*ker_ads8341_adc_getPerodicData_func_t)(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t count);

typedef int8_t (*ker_ads8341_adc_stopPerodicData_func_t)(uint8_t port);

#endif // _ADS8341_FUNC_ADC_H_

