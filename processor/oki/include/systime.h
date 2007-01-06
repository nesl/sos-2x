/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _SYSTIME_H
#define _SYSTIME_H

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
