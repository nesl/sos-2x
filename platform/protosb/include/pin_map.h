/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#ifndef _PIN_MAP_H_
#define _PIN_MAP_H_

#include <pin_defs.h>
#include <pin_alt_func.h>
		
/**
 * PORTA: external memory address/data
 * mapping for diagonistcs
 */
ALIAS_IO_PIN( SH15_CLK, PINA5);
ALIAS_IO_PIN( PREAMP_SHDN, PINA4);
ALIAS_IO_PIN( AMP_CS, PINA3);
ALIAS_IO_PIN( ADC_CS, PINA2);
ALIAS_IO_PIN( PROTO_SHDN, PINA1);
ALIAS_IO_PIN( VREF_SHDN, PINA0);

/* NOTE: PWR_CTRL pin ordering reversed to improve readablilty */
ALIAS_IO_PIN( PWR_CTRL0, PINC7);
ALIAS_IO_PIN( PWR_CTRL1, PINC6);
ALIAS_IO_PIN( PWR_CTRL2, PINC5);
ALIAS_IO_PIN( PWR_CTRL3, PINC4);
ALIAS_IO_PIN( V_SEL1, PINC3);
ALIAS_IO_PIN( V_SEL0, PINC2);
ALIAS_IO_PIN( EXT_PWR_EN1, PINC1);
ALIAS_IO_PIN( EXT_PWR_EN0, PINC0);

/** USART1 */
ALIAS_IO_PIN( AMP_SHDN, PIND7);
ALIAS_IO_PIN( ADC_SHDN, PIND6);

/** External Interrupt[7:4] Inputs */
ALIAS_IO_PIN( MICA2_INT3, INT7); // ext interrupt 7
ALIAS_IO_PIN( MICA2_INT2, INT6); // ext interrupt 6
ALIAS_IO_PIN( MICA2_INT1, INT5); // ext interrupt 5
ALIAS_IO_PIN( SH15_DATA, PINE5);
ALIAS_IO_PIN( MICA2_INT0, INT4); // ext interrupt 4
ALIAS_IO_PIN( ADC_BUSY, PINE4);

/** initalizadion function for I/O pins */
void init_IO(void);

#endif // _PIN_MAP_H_
