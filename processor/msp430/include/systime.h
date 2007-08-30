/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _SYSTIME_H
#define _SYSTIME_H

static inline uint32_t ticks_to_msec(uint32_t ticks) {
  return ((ticks >> 5) * 1000) >> 10;
}

#ifdef TMOTE_PLATFORM
/**
 * The following parameters are TMote specific.
 * The TMote uses the 16bit TimerB of the msp430 for system time, but
 * ker_systime32 still returns a 32bit counter since we use the overflow
 * interrupt to track the higher par.
 *
 **/

//#define PROPAGATION_DELAY_FLOAT 
#define AVG_PROPAGATION_DELAY 100 // this is the propagation delay in ticks, needs to be measured.
#define SIGMA 10 // not sure about that
#define INT_MAX_GTIME 0x7F000000
//#define FLOAT_MAX_GTIME (((0x7F000000 >> 5) * 1000) >> 10) // 0x7F000000 in milliseconds

#endif

/**
 * @brief initialize systime kernel device
 */
void systime_init();

#ifndef _MODULE_
/**
 * @brief terminate systime kernel device
 */
void systime_stop();

/**
 * @brief get systime in 32 bits
 */
uint32_t ker_systime32();

/**
 * @brief get systime in 16 bits
 */
uint16_t ker_systime16L();

/**
 * @brief get systime in upper 16 bits
 */
uint16_t ker_systime16H();
#endif
#endif
