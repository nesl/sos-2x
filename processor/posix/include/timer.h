#ifndef _SOS_TIMER_H
#define _SOS_TIMER_H

#define PROCESSOR_TICKS(x) x

/**
 * @brief Timer task
 */
extern void timer_hardware_init(uint8_t interval, uint8_t scale);
extern void timer_hardware_terminate( void );
extern void timer_setInterval(int32_t value);
extern void timer_int_(void);
#define timer_interrupt()        void timer_int_(void)
extern uint8_t timer_getInterval();
extern uint8_t timer_hardware_get_counter();

#endif // _SOS_TIMER_H

