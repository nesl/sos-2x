/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */


#ifndef _PIN_DEFS_H_
#define _PIN_DEFS_H_

#include <io.h>
/** 
 * Port/Pin naming scheme from the MSP430 datasheet.
 * If using msp340-libc all names are declared in gpio.h 
 * which is included from io.h
 */
/**
 * Port names
 * 
 * PxIN:	Input
 * PxOUT:	Output
 * PxDIR:	Direction
 * PxIFG:	Interrupt Flag
 * PxIES:	Interrupt Edge Select
 * PxIE:	Interrupt Enable
 * PxSEL:	Port Function Select
 */


#define DEF_IO_PIN(name, port, pin)										\
  static inline uint8_t name##_PORT() { return P##port##OUT_; }			\
  static inline uint8_t name##_BIT() { return pin; }					\
																		\
  static inline int READ_##name()										\
  { return (P##port##IN & (1 << pin)) ? 1 : 0; }						\
  static inline int name##_IS_SET()										\
  { return (P##port##IN & (1 << pin)) ? 1 : 0; }						\
  static inline int name##_IS_CLR()										\
  { return (P##port##IN & (1 << pin)) ? 0 : 1; }						\
																		\
  static inline void SET_##name() { P##port##OUT |= (1 << pin); }		\
  static inline void CLR_##name() { P##port##OUT &= (~(1 << pin )); }	\
  static inline void TOGGLE_##name() { P##port##OUT ^= (1 << pin ); }	\
																		\
  static inline void SET_##name##_DD_OUT() { P##port##DIR |= (1 << pin); } \
  static inline void SET_##name##_DD_IN() { P##port##DIR &= ~(1 << pin); } \
  																		\
  static inline void name##_SET_ALT() { P##port##SEL |= (1 << pin); }	\
  static inline void name##_UNSET_ALT() { P##port##SEL &= ~(1 << pin); }


#define DEF_OUTPUT_PIN(name, port, pin)									\
  static inline uint8_t name##_PORT() { return P##port##OUT_; }			\
  static inline uint8_t name##_BIT() { return pin; }					\
																		\
  static inline int name##_IS_SET()										\
  { return (P##port##IN & (1 << pin)) ? 1 : 0; }						\
  static inline int name##IS_CLR()										\
  { return (P##port##IN & (1 << pin)) ? 0 : 1; }						\
																		\
  static inline void SET_##name() { P##port##OUT |= (1 << pin); }		\
  static inline void CLR_##name() { P##port##OUT &= (~(1 << pin )); }	\
  static inline void TOGGLE_##name() { P##port##OUT ^= (1 << pin ); }	\
																		\
  static inline void SET_##name##_DD_OUT() { P##port##DIR |= (1 << pin); } \
																		\
  static inline void name##_SET_ALT() { P##port##SEL |= (1 << pin); }	\
  static inline void name##_UNSET_ALT() { P##port##SEL &= ~(1 << pin); } \
																		\
  static inline void name##_CS_HIGH() { P##port##OUT |= (1 << pin); }	\
  static inline void name##_CS_LOW() { P##port##OUT &= (~(1 << pin )); }


#define ALIAS_IO_PIN(alias, pin)										\
  static inline uint8_t alias##_PORT() { return pin##_PORT(); }			\
  static inline uint8_t alias##_BIT() { return pin##_BIT(); }			\
																		\
  static inline int READ_##alias() { return READ_##pin(); }				\
  static inline int alias##_IS_SET() { return pin##_IS_SET(); }			\
  static inline int alias##_IS_CLR() { return pin##_IS_CLR(); }			\
																		\
  static inline void SET_##alias() { SET_##pin(); }						\
  static inline void CLR_##alias() { CLR_##pin(); }						\
  static inline void TOGGLE_##alias() { TOGGLE_##pin(); }				\
																		\
  static inline void SET_##alias##_DD_IN() { SET_##pin##_DD_IN(); }		\
  static inline void SET_##alias##_DD_OUT() { SET_##pin##_DD_OUT(); }	\
																		\
  static inline void alias##_SET_ALT() { pin##_SET_ALT(); }				\
  static inline void alias##_UNSET_ALT() { pin##_UNSET_ALT(); }


#define ALIAS_OUTPUT_PIN(alias, pin)								\
  static inline uint8_t alias##_PORT() { return pin##_PORT(); }		\
  static inline uint8_t alias##_BIT() { return pin##_BIT(); }		\
  																	\
  static inline int alias##_IS_SET() { return pin##_IS_SET(); }		\
  static inline int alias##_IS_CLR() { return pin##_IS_CLR(); }		\
  																	\
  static inline void SET_##alias() { SET_##pin(); }					\
  static inline void CLR_##alias() { CLR_##pin(); }					\
  static inline void TOGGLE_##alias() { TOGGLE_##pin(); }			\
  																	\
  static inline void SET_##alias##_DD_IN() { SET_##pin##_DD_IN(); }		\
  static inline void SET_##alias##_DD_OUT() { SET_##pin##_DD_OUT(); }	\
																	\
  static inline void alias##_SET_ALT() { name##_SET_ALT(); }		\
  static inline void alias##_UNSET_ALT() { name##_UNSET_ALT(); }	\
																	\
  static inline void alias##_CS_HIGH() { SET_##pin(); }				\
  static inline void alias##_CS_LOW() { CLR_##pin(); }


/**
 * PORT1:
 */
DEF_IO_PIN( P1_0, 1, 0);
DEF_IO_PIN( P1_1, 1, 1);
DEF_IO_PIN( P1_2, 1, 2);
DEF_IO_PIN( P1_3, 1, 3);
DEF_IO_PIN( P1_4, 1, 4);
DEF_IO_PIN( P1_5, 1, 5);
DEF_IO_PIN( P1_6, 1, 6);
DEF_IO_PIN( P1_7, 1, 7);

/**
 * PORT2:
 */
DEF_IO_PIN( P2_0, 2, 0);
DEF_IO_PIN( P2_1, 2, 1);
DEF_IO_PIN( P2_2, 2, 2);
DEF_IO_PIN( P2_3, 2, 3);
DEF_IO_PIN( P2_4, 2, 4);
DEF_IO_PIN( P2_5, 2, 5);
DEF_IO_PIN( P2_6, 2, 6);
DEF_IO_PIN( P2_7, 2, 7);

/**
 * PORT3:
 */
DEF_IO_PIN( P3_0, 3, 0);
DEF_IO_PIN( P3_1, 3, 1);
DEF_IO_PIN( P3_2, 3, 2);
DEF_IO_PIN( P3_3, 3, 3);
DEF_IO_PIN( P3_4, 3, 4);
DEF_IO_PIN( P3_5, 3, 5);
DEF_IO_PIN( P3_6, 3, 6);
DEF_IO_PIN( P3_7, 3, 7);

/**
 * PORT4:
 */
DEF_IO_PIN( P4_0, 4, 0);
DEF_IO_PIN( P4_1, 4, 1);
DEF_IO_PIN( P4_2, 4, 2);
DEF_IO_PIN( P4_3, 4, 3);
DEF_IO_PIN( P4_4, 4, 4);
DEF_IO_PIN( P4_5, 4, 5);
DEF_IO_PIN( P4_6, 4, 6);
DEF_IO_PIN( P4_7, 4, 7);

/**
 * PORT5:
 */
DEF_IO_PIN( P5_0, 5, 0);
DEF_IO_PIN( P5_1, 5, 1);
DEF_IO_PIN( P5_2, 5, 2);
DEF_IO_PIN( P5_3, 5, 3);
DEF_IO_PIN( P5_4, 5, 4);
DEF_IO_PIN( P5_5, 5, 5);
DEF_IO_PIN( P5_6, 5, 6);
DEF_IO_PIN( P5_7, 5, 7);

/**
 * PORT6:
 */
DEF_IO_PIN( P6_0, 6, 0);
DEF_IO_PIN( P6_1, 6, 1);
DEF_IO_PIN( P6_2, 6, 2);
DEF_IO_PIN( P6_3, 6, 3);
DEF_IO_PIN( P6_4, 6, 4);
DEF_IO_PIN( P6_5, 6, 5);
DEF_IO_PIN( P6_6, 6, 6);
DEF_IO_PIN( P6_7, 6, 7);

#endif	// _PIN_DEFS_H_
