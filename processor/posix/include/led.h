/**
 * @brief    header file for LED
 * @author   Simon Han
 */


#ifndef _LED_H
#define _LED_H

#define LED_RED_ON           1
#define LED_GREEN_ON         2
#define LED_YELLOW_ON        3
#define LED_RED_OFF          4
#define LED_GREEN_OFF        5
#define LED_YELLOW_OFF       6
#define LED_RED_TOGGLE       7
#define LED_GREEN_TOGGLE     8
#define LED_YELLOW_TOGGLE    9

/**
 * kernel writer can just use macros provided by SOS
 *
 * led_red_on()        
 * led_green_on()    
 * led_yellow_on()   
 * led_red_off()      
 * led_green_off()     
 * led_yellow_off()    
 * led_red_toggle() 
 * led_green_toggle() 
 * led_yellow_toggle()
 */
/**
 * @brief led macros
 */
#define led_red_on()         ker_led(LED_RED_ON)
#define led_green_on()       ker_led(LED_GREEN_ON)
#define led_yellow_on()      ker_led(LED_YELLOW_ON)
#define led_red_off()        ker_led(LED_RED_OFF)
#define led_green_off()      ker_led(LED_GREEN_OFF)
#define led_yellow_off()     ker_led(LED_YELLOW_OFF)
#define led_red_toggle()     ker_led(LED_RED_TOGGLE)
#define led_green_toggle()   ker_led(LED_GREEN_TOGGLE)
#define led_yellow_toggle()  ker_led(LED_YELLOW_TOGGLE)

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_led(uint8_t action);
extern int8_t led_init();
#endif /* _MODULE_ */
#endif

