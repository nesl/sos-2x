/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _SYSTIME_H
#define _SYSTIME_H

/**
 * @brief constants used by TPSN and RATS
 */

// The following value is used to calculate between Timer3 clock ticks (that are returned
// by ker_systime32() and milliseconds. It is Timer3's frequency in 10*KHz.
#define SYSTIME_FREQUENCY	1152

uint32_t ticks_to_msec(uint32_t ticks);
uint32_t msec_to_ticks(uint32_t msec);
 
// 1230 microseconds is the propogation delay (154 clock ticks, if the resolution
// of the timer is 8 microseconds/tick)
// SIGMA shows the normal variance of the propagation delay. It is 0.32 microseconds 
// if the resolution of the timer is 1 microseconds/tick). We are using a resolution of
// 8 microseconds/tick that's why it's set to 10 (instead of 4)
// These values are specific for Mica2 and the underlying timestamping library
// Needs to be changed accordingly
// 

#define PROPAGATION_DELAY_FLOAT 1.23F
#define AVG_PROPAGATION_DELAY 154
#define SIGMA 10
#define TIMER3_MAX_GTIME (0x7F00)
#define INT_MAX_GTIME (TIMER3_MAX_GTIME*0x10000)
#define FLOAT_MAX_GTIME (18495715) //0x7F000000 in milliseconds

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
