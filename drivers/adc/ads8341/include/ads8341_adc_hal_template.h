/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief example hardware abstraction layer for the ADS8341
 * @author Naim Busek <ndbusek@gmail.com>
 */

/**
 * to use this component your platform must define the platform specific
 * IO and timer
 *
 * this is done by creating a ads8341_adc_hal.h file in your platform
 * directory that defines the following necessary hardware abstractions
 *
 * the following is an example configuration from the protosb platform
 * 
 * please use the values defined in the device header file when defining
 * these values
 */

#ifndef _ADS8341_ADC_HAL_H_
#define _ADS8341_ADC_HAL_H_

// !SHDN
#define ads8341_adc_on()	SET_ADC_SHDN()
#define ads8341_adc_off()	CLR_ADC_SHDN()

/* adc bush interrupt handler */
#define ads8341_adc_busy_interrupt()        SIGNAL(SIG_INTERRUPT4)

//#define ads8341_adc_config_interrupt()     (EICRB &= ~((1<<ISC41)|(1<<ISC40))) // low level triggered
//#define ads8341_adc_config_interrupt()     (EICRB &= ~(1<<ISC41); EICRB |= (1<<ISC40)) // any edge triggered
//#define ads8341_adc_config_interrupt()     (EICRB |= (1<<ISC41); EICRB &= ~(1<<ISC40);) // falling edge triggered
#define ads8341_adc_config_interrupt()      (EICRB |= ((1<<ISC41)|(1<<ISC40))) // rising edge triggered
#define ads8341_adc_enable_interrupt()      (EIMSK |= (1<<INT4))
#define ads8341_adc_disable_interrupt()     (EIMSK &= ~(1<<INT4))

/* interrupt handler */
#define ads8341_adc_clk_interrupt()         SIGNAL(SIG_OUTPUT_COMPARE2)

#define ads8341_adc_clk_enable_interrupt()  (TIMSK |= (1<<TOIE2)|(1<<OCIE2))
#define ads8341_adc_clk_disable_interrupt() (TIMSK &= ~((1<<TOIE2)|(1<<OCIE2)))

/* adc sample interval clock */
#define ads8341_adc_clk_set(val)            (TCNT2 = (val))
#define ads8341_adc_clk_set_prescale(val)   (TCCR2 |= (val))
#define ads8341_adc_clk_set_interval(val)   (OCR2 = val)

#define ads8341_adc_clk_stop()              (TCCR2 &= ~((1<<CS2)|(1<<CS1)|(1<<CS0)))

// platform specific hardware initalization
extern int8_t ads8341_adc_hardware_init(void);

#endif // _ADS8341_ADC_HAL_H_

