/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */


#ifndef _TIMER_H
#define _TIMER_H
#include <signal.h>
#include <timerb_hal.h>

/**
 * @note June 22,2006 Ram - I am shifting the counter value by 6 as opposed to 5.
 * @todo June 22,2006 Ram - Resolve the timer jitter issue. Still very bad
 */

/**
 * @brief Initialize the timer hardware
 */
extern void timer_hardware_init(uint8_t interval);

/**
 * @brief Set a new value for the timer interval
 * @param val The value of the new interval
 */
extern void timer_setInterval(int32_t val);

// Timer Interrupt from hardware
void timerb_compare3_interrupt();
#define timer_interrupt() void timerb_compare3_interrupt()


/**
 * @brief Get the current interval setting
 */ 
//extern uint16_t _timerb_prev_tbr;
extern uint8_t _timerb_interval;
static inline uint8_t timer_getInterval()     {return (uint8_t)_timerb_interval;}
extern uint8_t timer_hardware_get_counter();
/**
 * @brief Get the current value stored in the hardware register
 */
/*
static inline uint8_t timer_hardware_get_counter() {
  uint8_t retval;
  uint16_t temp;
  temp = timerb_reliable_read() - _timerb_prev_tbr;
  //  retval = (uint8_t)(temp >> 6);
  retval = (uint8_t)(temp >> 5);
  //  retval = (uint8_t)(temp);
  return retval;
}
*/




#endif // _SOS_TIMER_H

