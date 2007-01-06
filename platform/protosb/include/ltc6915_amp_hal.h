/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <hardware.h>
      
#ifndef _LTC6915_AMP_HAL_H_
#define _LTC6915_AMP_HAL_H_

static inline void ltc6915_amp_hal_on() { CLR_AMP_SHDN(); }
static inline void ltc6915_amp_hal_off() { SET_AMP_SHDN(); }

static inline spi_addr_t ltc6915_amp_hal_get_addr() {
	spi_addr_t addr;

	addr.cs_reg = AMP_CS_PORT();
	addr.cs_bit = AMP_CS_BIT();

	return addr;
}

/** @brief amplifire hardware init */
extern int8_t ltc6915_amp_hardware_init(void);

#endif // _LTC6915_AMP_HAL_H_

