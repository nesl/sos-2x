/**
 * \file avr_minielf.h
 * \brief AVR specific ELF data types and data structures
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _AVR_MINIELF_H_
#define _AVR_MINIELF_H_

#include <inttypes.h>
#include <sos_endian.h>

typedef uint8_t  Melf_Half;
typedef uint16_t Melf_Word;
typedef uint16_t Melf_Addr;
typedef uint16_t Melf_Off;
typedef uint16_t Melf_Sword;

// EHTON
#define ehton_Melf_Half(x)  x
#define ehton_Melf_Word(x)  ehtons(x)
#define ehton_Melf_Addr(x)  ehtons(x)
#define ehton_Melf_Off(x)   ehtons(x)
#define ehton_Melf_Sword(x) ehtons(x)
// ENTOH
#define entoh_Melf_Half(x)  x
#define entoh_Melf_Word(x)  entohs(x)
#define entoh_Melf_Addr(x)  entohs(x)
#define entoh_Melf_Off(x)   entohs(x)
#define entoh_Melf_Sword(x) entohs(x)

/*
 * Relocation Types
 */
#define R_AVR_NONE 		0
#define R_AVR_32 		1
#define R_AVR_7_PCREL 		2
#define R_AVR_13_PCREL 		3
#define R_AVR_16 	        4
#define R_AVR_16_PM 		5
#define R_AVR_LO8_LDI 		6
#define R_AVR_HI8_LDI 		7
#define R_AVR_HH8_LDI 		8
#define R_AVR_LO8_LDI_NEG 	9
#define R_AVR_HI8_LDI_NEG      10
#define R_AVR_HH8_LDI_NEG      11
#define R_AVR_LO8_LDI_PM       12
#define R_AVR_HI8_LDI_PM       13
#define R_AVR_HH8_LDI_PM       14
#define R_AVR_LO8_LDI_PM_NEG   15
#define R_AVR_HI8_LDI_PM_NEG   16
#define R_AVR_HH8_LDI_PM_NEG   17
#define R_AVR_CALL 	       18



#endif//_AVR_MINIELF_H_
