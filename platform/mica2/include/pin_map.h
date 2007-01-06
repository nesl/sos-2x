/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
#ifndef _PIN_MAP_H
#define _PIN_MAP_H

#include <pin_defs.h>
#include <pin_alt_func.h>


// LED assignments
ALIAS_IO_PIN( RED_LED, PINA2);
ALIAS_IO_PIN( GREEN_LED, PINA1);
ALIAS_IO_PIN( YELLOW_LED, PINA0);

ALIAS_IO_PIN( SERIAL_ID, PINA4);
ALIAS_IO_PIN( BAT_MON, PINA5);
ALIAS_IO_PIN( THERM_PWR, PINA7);

// ChipCon control assignments
ALIAS_IO_PIN( CC_CHP_OUT, PINA6); // chipcon CHP_OUT
ALIAS_IO_PIN( CC_PDATA, PIND7);  // chipcon PDATA 
ALIAS_IO_PIN( CC_PCLK, PIND6);	  // chipcon PCLK
ALIAS_IO_PIN( CC_PALE, PIND4);	  // chipcon PALE

// Flash assignments
ALIAS_IO_PIN( FLASH_SELECT, PINA3);
ALIAS_IO_PIN( FLASH_CLK, PIND5);
ALIAS_IO_PIN( FLASH_OUT, PIND3);
ALIAS_IO_PIN( FLASH_IN, PIND2);

// interrupt assignments
ALIAS_IO_PIN( MICA2_INT0, INT4);
ALIAS_IO_PIN( MICA2_INT1, INT5);
ALIAS_IO_PIN( MICA2_INT2, INT6);
ALIAS_IO_PIN( MICA2_INT3, INT7);

// power control assignments
ALIAS_IO_PIN( PW0, PINC0);
ALIAS_IO_PIN( PW1, PINC1);
ALIAS_IO_PIN( PW2, PINC2);
ALIAS_IO_PIN( PW3, PINC3);
ALIAS_IO_PIN( PW4, PINC4);
ALIAS_IO_PIN( PW5, PINC5);
ALIAS_IO_PIN( PW6, PINC6);
ALIAS_IO_PIN( PW7, PINC7);

void init_IO(void);

#endif //_PIN_MAP_H

