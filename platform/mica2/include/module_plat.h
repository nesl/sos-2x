/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/** 
 * @brief Header file for mica2 specific modules
 */
#ifndef _MODULE_PLAT_H
#define _MODULE_PLAT_H

#include <kertable_plat.h>

#ifdef _MODULE_

/**
 * Radio Link Layer Ack
 */
typedef void (*ker_radio_ack_func_t)(void);
/** Enable Ack */
static inline void ker_radio_ack_enable(void)
{   
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(PROC_KERTABLE_END+1);
	func();
	return;
}
/** Disable Ack */
static inline void ker_radio_ack_disable(void)
{
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(PROC_KERTABLE_END+2);
	func();
	return;
}  


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
typedef int8_t (*ledfunc_t)(uint8_t action);
static inline int8_t ker_led(uint8_t action){
	ledfunc_t func = (ledfunc_t)get_kertable_entry(PROC_KERTABLE_END+3);
	return func(action);
}


// TODO: There is probably a better place for this.  I will move it as soon as
// I figuer out what that better place is... (Roy)
typedef int8_t (*ker_one_wire_write_t)(uint8_t *command_data, 
    uint8_t command_size, 
    uint8_t *write_data, 
    uint8_t write_size);
static inline int8_t ker_one_wire_write(uint8_t *command_data,
    uint8_t command_size,
    uint8_t *write_data,
    uint8_t write_size) {
	ker_one_wire_write_t func = (ker_one_wire_write_t)get_kertable_entry(PROC_KERTABLE_END+4);
	return func(command_data, command_size, write_data, write_size);
}

// TODO: There is probably a better place for this.  I will move it as soon as
// I figuer out what that better place is... (Roy)
typedef int8_t (*ker_one_wire_read_t)(uint8_t *command_data, 
    uint8_t command_size, 
    uint8_t *read_data, 
    uint8_t read_size);
static inline int8_t ker_one_wire_read(uint8_t *command_data,
    uint8_t command_size,
    uint8_t *read_data,
    uint8_t read_size) {
	ker_one_wire_read_t func = (ker_one_wire_read_t)get_kertable_entry(PROC_KERTABLE_END+5);
	return func(command_data, command_size, read_data, read_size);
}


#endif /* _MODULE_ */
#endif /* _MODULE_PLAT_H */

