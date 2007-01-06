

/**
 * @brief timer hardware routine
 */
#include <hardware.h>
#include <sys/time.h>
#include <time.h>
#include "timer.h"
#include <sos_timer.h>

#undef DEBUG
#define DEBUG(...)

static uint8_t timer_interval;

/**
 * @brief fire interrupt if counter value reachs interval
 */
static void fire_interrupt()
{
	DEBUG("timer interrupt\n");
	timer_int_();
}


void timer_hardware_init(uint8_t interval, uint8_t scale){

    timer_init();
}


void timer_hardware_terminate( void ) 
{
}

void timer_setInterval(int32_t value)
{
	DEBUG("timer set interval %d\n", value);	
	if(value > 250) value = 250;
	if(value < 1) value = 1;
	timer_interval = (uint8_t) value;
	interrupt_set_timeout(value, fire_interrupt);
}

uint8_t timer_hardware_get_counter()
{
	int ret = interrupt_get_elapsed_time();

	DEBUG("timer get counter %d\n", ret);
	if( ret > 250 ) {
		return 250;
	} 
	return (uint8_t)ret;
}

uint8_t timer_getInterval()
{
	DEBUG("timer get interval %d\n", timer_interval);
	return (uint8_t)(timer_interval);
}

