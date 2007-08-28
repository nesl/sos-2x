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


// TODO: There is probably a better place for this.  I will move it as soon as
// I figuer out what that better place is... (Roy)
extern int8_t ker_one_wire_write(uint8_t *command_data,
    uint8_t command_size,
    uint8_t *write_data,
    uint8_t write_size); 

// TODO: There is probably a better place for this.  I will move it as soon as
// I figuer out what that better place is... (Roy)
extern int8_t ker_one_wire_read(uint8_t *command_data,
    uint8_t command_size,
    uint8_t *read_data,
    uint8_t read_size); 

#endif /* _MODULE_PLAT_H */

