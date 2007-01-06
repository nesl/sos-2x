/**
 * \file msp430_minielf.h
 * \brief MSP430  specific ELF data types and data structures
 * \author Simon Han
 */

#ifndef _MSP430_MINIELF_H_
#define _MSP430_MINIELF_H_

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
#define       R_MSP430_NONE      0
#define       R_MSP430_32        1
#define       R_MSP430_10_PCREL  2
#define       R_MSP430_16        3
#define       R_MSP430_16_PCREL  4
#define       R_MSP430_16_BYTE   5
#define       R_MSP430_16_PCREL_BYTE   6


#endif //_MSP430_MINIELF_H_
