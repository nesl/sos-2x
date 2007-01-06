/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _ADC_CB_H_
#define _ADC_CB_H_

typedef int8_t (*adc16_cb_t)(func_cb_ptr p, uint8_t port, uint16_t value, uint8_t status);
typedef int8_t (*adc12_cb_t)(func_cb_ptr p, uint8_t port, uint16_t value, uint8_t status);
typedef int8_t (*adc10_cb_t)(func_cb_ptr p, uint8_t port, uint16_t value, uint8_t status);

#endif // _ADC_CB_H_
