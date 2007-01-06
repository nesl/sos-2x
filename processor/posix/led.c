/**
 * @brief    led module for mica2
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  0.1
 *
 */

#include <sos.h>
#include <led.h>

#ifndef SOS_DEBUG_LEDS
#undef DEBUG
#define DEBUG(...) 
#endif

uint8_t __red_led, __green_led, __yellow_led;

int8_t ker_led(uint8_t action){
	switch (action){
     case LED_RED_ON:
     {
        __red_led = 1;
        DEBUG("LED: RED ON\n");
        break;
     }
     
     case LED_GREEN_ON:
     {
        __green_led = 1;
        DEBUG("LED: GREEN ON\n");
        break;
     }
     
     case LED_YELLOW_ON:
     {
        __yellow_led = 1;
        DEBUG("LED:YELLOW ON\n");
        break;
     }
     
     
     case LED_RED_OFF:
     {
        __red_led = 0; 
        DEBUG("LED: RED OFF\n");
        break;
     }
     
     case LED_GREEN_OFF:
     {
        __green_led = 0; 
        DEBUG("LED: GREEN OFF\n");
        break;
     }
     
     case LED_YELLOW_OFF:
     {
        __yellow_led = 0;
        DEBUG("LED:YELLOW OFF\n");
        break;
     }
     
     case LED_RED_TOGGLE:
     {
        __red_led ^= 0x01; 
        if (__red_led)
        {
           DEBUG("LED: RED ON\n");
        }
        else
        {
           DEBUG("LED: RED OFF\n");
        }
        break;
     }
     
     case LED_GREEN_TOGGLE:
     {
        __green_led ^= 0x01; 
        if (__green_led)
        {
           DEBUG("LED: GREEN ON\n");
        }
        else
        {
           DEBUG("LED: GREEN OFF\n");
        }
        break;
     }  
     
     case LED_YELLOW_TOGGLE:
     {
        __yellow_led ^= 0x01; 
        if (__yellow_led)
        {
           DEBUG("LED: YELLOW ON\n");
        }
        else
        {
           DEBUG("LED: YELLOW OFF\n");
        }
        break;
     }
	}
	return SOS_OK;
}


int8_t led_init(void){
   __red_led = 0;
   __green_led = 0;
   __yellow_led = 0;
   return SOS_OK;
}     
