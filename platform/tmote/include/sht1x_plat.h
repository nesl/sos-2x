#ifndef _SHT1x_PLAT_H_
#define _SHT1x_PLAT_H_

#include <bitsop.h>

/** 
 * SHT1x (SHT11) data and clock line operations
 *
 * \note The SHT1x uses a modified version of the I2C interface.
 *
 */

#define SHT1x_PORT 			P1
#define SHT1x_DIRECTION 	P1DIR
#define SHT1x_READ_PIN 		P1IN
#define SHT1x_PIN_NUMBER	5
#define SHT1x_PIN 			0x10

#define set_data()           SETBITHIGH(P1OUT, 5)
#define clear_data()         SETBITLOW(P1OUT, 5)
#define set_clock()          SETBITHIGH(P1OUT, 6)
#define clear_clock()        SETBITLOW(P1OUT, 6)
#define enable_sht1x()       SETBITHIGH(P1OUT, 7)
#define disable_sht1x()      SETBITLOW(P1OUT, 7)
#define make_data_output()   SETBITHIGH(P1DIR, 5)
#define make_data_input()    SETBITLOW(P1DIR, 5)
#define make_clock_output()  SETBITHIGH(P1DIR, 6)
#define make_enable_output() SETBITHIGH(P1DIR, 7)
#define select_enable_io()	 SETBITLOW(P1SEL, 7)

#define SHT1x_TEMPERATURE_TIME 240L
#define SHT1x_HUMIDITY_TIME 70L

#endif

