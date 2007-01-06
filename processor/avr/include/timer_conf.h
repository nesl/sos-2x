
#ifndef _TIMER_CONF_H
#define _TIMER_CONF_H

// careful with these defines that is a zero not an O.
// the second number is a divesor on the clock source
// read them as: Timer0 clock/##
#define CLK_OFF	 0
#define CLK_1	 1
#define CLK_8	 2
#define CLK_32	 3
#define CLK_64	 4
#define CLK_128	 5
#define CLK_256	 6
#define CLK_1024 7

#define DEFAULT_INTERVAL        0x10
#define DEFAULT_SCALE           CLK_32
#define TIMER_MIN_INTERVAL      5
#define MAX_PRE_ALLOCATED_TIMERS 10 //! Keep this an even value to make the mod_header_t structure 16 bits aligned

#endif
