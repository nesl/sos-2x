/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/** 
 * @brief Header file for mica2 specific modules
 */
#ifndef _MODULE_PLAT_H
#define _MODULE_PLAT_H


/**
 * Radio Link Layer Ack
 */
/** Enable Ack */
extern void ker_radio_ack_enable(void);
/** Disable Ack */
extern void ker_radio_ack_disable(void);


/**
 * @brief led functions
 * @param action can be following
 *    LED_RED_ON
 *    LED_GREEN_ON
 *    LED_YELLOW_ON
 *    LED_RED_OFF
 *    LED_GREEN_OFF
 *    LED_YELLOW_OFF
 *    LED_RED_TOGGLE
 *    LED_GREEN_TOGGLE
 *    LED_YELLOW_TOGGLE
 */
extern int8_t ker_led(uint8_t action);

#endif /* _MODULE_PLAT_H */

