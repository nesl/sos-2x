/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
      
#ifndef _LTC6915_AMP_H_
#define _LTC6915_AMP_H_

/** @brief driver init function */
extern int8_t ltc6915_amp_init();

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_ltc6915_amp_setGain(uint8_t calling_id, uint8_t gain);
#endif

#endif // _LTC6915_AMP_H_

