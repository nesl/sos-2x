/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */


#ifndef _PIN_DEFS_H_
#define _PIN_DEFS_H_

#include <avr/io.h>
/** 
 * port/pin naming scheme from the atmega128 datasheet.
 * if using avr-libc all names are declared in iom128.h which is included from io.h
 */
/**
 * Port names
 *
 * PORTx:	Data Register
 * DDRx:	Data Direction Register
 * PINx:	Port Input Pins
 */

/**
 * Pin names
 *
 * DDxn: 	Data Direction Bit
 * PORTxn: 	Data Bit
 * PINxn:	Pin Input Bit
 */

/**
 * Global IO controls
 * 
 * SLEEP: SLEEP CONTROL
 * PUD: PULLUP DISABLE
 * clkI/O: I/O CLOCK
 */

/**
 * Port specific controls
 * 
 * WDx: WRITE DDRx
 * RDx: READ DDRx
 * WPx: WRITE PORTx
 * RRx: READ PORTx REGISTER
 * RPx: READ PORTx PIN
 */

/**
 * Pin specific controls
 * 
 * PUOExn: Pxn PULL-UP OVERRIDE ENABLE
 * PUOVxn: Pxn PULL-UP OVERRIDE VALUE
 * DDOExn: Pxn DATA DIRECTION OVERRIDE ENABLE
 * DDOVxn: Pxn DATA DIRECTION OVERRIDE VALUE
 * PVOExn: Pxn PORT VALUE OVERRIDE ENABLE
 * PVOVxn: Pxn PORT VALUE OVERRIDE VALUE
 * DIxn: DIGITAL INPUT PIN n ON PORTx
 * AIOxn: ANALOG INPUT/OUTPUT PIN n ON PORTx
 * DIEOExn: Pxn DIGITAL INPUT-ENABLE OVERRIDE ENABLE
 * DIEOVxn: Pxn DIGITAL INPUT-ENABLE OVERRIDE VALUE
 * DIxn: DIGITAL INPUT PIN n ON PORTx
 * AIOxn: ANALOG INPUT/OUTPUT PIN n ON PORTx
 */


#define DEF_IO_PIN(name, port, pin) \
	static inline uint8_t name##_PORT() { return _SFR_ADDR(PORT##port); } \
	static inline uint8_t name##_BIT() { return pin; } \
	\
	static inline int8_t READ_##name() \
		{ return (PIN##port & (1 << pin)) ? 1 : 0; } \
	static inline int8_t name##_IS_SET() \
		{ return (PORT##port & (1 << pin)) ? 1 : 0; } \
	static inline int8_t name##_IS_CLR() \
		{ return (PORT##port & (1 << pin)) ? 0 : 1; } \
	\
	static inline void SET_##name() { PORT##port |= (1 << pin); } \
	static inline void CLR_##name() { PORT##port &= (~(1 << pin )); } \
	static inline void TOGGLE_##name() { PORT##port ^= (1 << pin ); } \
	\
	static inline void SET_##name##_DD_OUT() { DDR##port |= (1 << pin); } \
	static inline void SET_##name##_DD_IN() { DDR##port &= ~(1 << pin); }


#define DEF_OUTPUT_PIN(name, port, pin) \
	static inline uint8_t name##_PORT() { return _SFR_ADDR(PORT##port); } \
	static inline uint8_t name##_BIT() { return pin; } \
	\
	static inline int8_t name##_IS_SET() \
		{ return (PORT##port & (1 << pin)) ? 1 : 0; } \
	static inline int8_t name##_IS_CLR() \
		{ return (PORT##port & (1 << pin)) ? 0 : 1; } \
	\
	static inline void SET_##name() { PORT##port |= (1 << pin); } \
	static inline void CLR_##name() { PORT##port &= (~(1 << pin )); } \
	static inline void TOGGLE_##name() { PORT##port ^= (1 << pin ); } \
	\
	static inline void SET_##name##_DD_OUT() { DDR##port |= (1 << pin); } \
	\
	static inline void name##_CS_HIGH() { PORT##port |= (1 << pin); } \
	static inline void name##_CS_LOW() { PORT##port &= (~(1 << pin )); }


#define ALIAS_IO_PIN(alias, pin) \
	static inline uint8_t alias##_PORT() { return pin##_PORT(); } \
	static inline uint8_t alias##_BIT() { return pin##_BIT(); } \
	\
	static inline int8_t READ_##alias() { return READ_##pin(); } \
	static inline int8_t alias##_IS_SET() { return pin##_IS_SET(); } \
	static inline int8_t alias##_IS_CLR() { return pin##_IS_CLR(); } \
	\
	static inline void SET_##alias() { SET_##pin(); } \
	static inline void CLR_##alias() { CLR_##pin(); } \
	static inline void TOGGLE_##alias() { TOGGLE_##pin(); } \
	\
	static inline void SET_##alias##_DD_IN() { SET_##pin##_DD_IN(); } \
	static inline void SET_##alias##_DD_OUT() { SET_##pin##_DD_OUT(); }
		

#define ALIAS_OUTPUT_PIN(alias, pin) \
	static inline uint8_t alias##_PORT() { return pin##_PORT(); } \
	static inline uint8_t alias##_BIT() { return pin##_BIT(); } \
	\
	static inline int8_t alias##_IS_SET() { return pin##_IS_SET(); } \
	static inline int8_t alias##_IS_CLR() { return pin##_IS_CLR(); } \
	\
	static inline void SET_##alias() { SET_##pin(); } \
	static inline void CLR_##alias() { CLR_##pin(); } \
	static inline void TOGGLE_##alias() { TOGGLE_##pin(); } \
	\
	static inline void SET_##alias##_DD_OUT() { SET_##pin##_DD_OUT(); } \
	\
	static inline void alias##_CS_HIGH() { SET_##pin(); } \
	static inline void alias##_CS_LOW() { CLR_##pin(); }


/**
 * PORTA:
 */
DEF_IO_PIN( PINA7, A, PA7);
DEF_IO_PIN( PINA6, A, PA6);
DEF_IO_PIN( PINA5, A, PA5);
DEF_IO_PIN( PINA4, A, PA4);
DEF_IO_PIN( PINA3, A, PA3);
DEF_IO_PIN( PINA2, A, PA2);
DEF_IO_PIN( PINA1, A, PA1);
DEF_IO_PIN( PINA0, A, PA0);

/**
 * PORTB:
 */
DEF_IO_PIN( PINB7, B, PB7);
DEF_IO_PIN( PINB6, B, PB6);
DEF_IO_PIN( PINB5, B, PB5);
DEF_IO_PIN( PINB4, B, PB4);
DEF_IO_PIN( PINB3, B, PB3);
DEF_IO_PIN( PINB2, B, PB2);
DEF_IO_PIN( PINB1, B, PB1);
DEF_IO_PIN( PINB0, B, PB0);

/**
 * PORTC:
 */
DEF_IO_PIN( PINC7, C, PC7);
DEF_IO_PIN( PINC6, C, PC6);
DEF_IO_PIN( PINC5, C, PC5);
DEF_IO_PIN( PINC4, C, PC4);
DEF_IO_PIN( PINC3, C, PC3);
DEF_IO_PIN( PINC2, C, PC2);
DEF_IO_PIN( PINC1, C, PC1);
DEF_IO_PIN( PINC0, C, PC0);


/**
 * PORTD:
 */
DEF_IO_PIN( PIND7, D, PD7);
DEF_IO_PIN( PIND6, D, PD6);
DEF_IO_PIN( PIND5, D, PD5);
DEF_IO_PIN( PIND4, D, PD4);
DEF_IO_PIN( PIND3, D, PD3);
DEF_IO_PIN( PIND2, D, PD2);
DEF_IO_PIN( PIND1, D, PD1);
DEF_IO_PIN( PIND0, D, PD0);


/**
 * PORTE:
 */
DEF_IO_PIN( PINE7, E, PE7);
DEF_IO_PIN( PINE6, E, PE6);
DEF_IO_PIN( PINE5, E, PE5);
DEF_IO_PIN( PINE4, E, PE4);
DEF_IO_PIN( PINE3, E, PE3);
DEF_IO_PIN( PINE2, E, PE2);
DEF_IO_PIN( PINE1, E, PE1);
DEF_IO_PIN( PINE0, E, PE0);

/**
 * PORTF:
 */
DEF_IO_PIN( PINF7, F, PF7);
DEF_IO_PIN( PINF6, F, PF6);
DEF_IO_PIN( PINF5, F, PF5);
DEF_IO_PIN( PINF4, F, PF4);
DEF_IO_PIN( PINF3, F, PF3);
DEF_IO_PIN( PINF2, F, PF2);
DEF_IO_PIN( PINF1, F, PF1);
DEF_IO_PIN( PINF0, F, PF0);

/**
 * PORTG:
 */
DEF_IO_PIN( PING4, G, PG4);
DEF_IO_PIN( PING3, G, PG3);
DEF_IO_PIN( PING2, G, PG2);
DEF_IO_PIN( PING1, G, PG1);
DEF_IO_PIN( PING0, G, PG0);

#endif	// _PIN_DEFS_H_
