/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief example hardware abstraction layer for the LTC6915 variable gain amplifier
 * @author Naim Busek <ndbusek@gmail.com>
 */

/**
 * to use this component your platform must define the platform specific IO
 *
 * this is done by creating a ltc6915_adc_hal.h file in your platform
 * directory that defines the following necessary hardware abstractions
 *
 * the following is an example configuration from the protosb platform
 * 
 * please use the values defined in the device header file when defining
 * these values
 */

#ifndef _LTC6915_AMP_HAL_H_
#define _LTC6915_AMP_HAL_H_

#define get_ltc6915_port() AMP_SHDN_PORT()
#define get_ltc6915_bit() AMP_SHDN_PIN()

#define ltc6915_amp_on() CLR_AMP_SHDN()
#define ltc6915_amp_off() SET_AMP_SHDN()

/** @brief amplifire hardware init */
extern int8_t ltc6915_amp_hardware_init();

#endif // _LTC6915_AMP_HAL_H_

