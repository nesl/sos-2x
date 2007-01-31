/**
 * \file soself.h
 * \brief Wrapper include file for Lib-ELF
 */

#ifndef _SOSELF_H_
#define _SOSELF_H_

#include <libelf.h>


/*
 * Platform Definitions
 */

#ifndef EM_AVR
#define EM_AVR		83	/* Atmel AVR 8-bit microcontroller */
#endif//EM_AVR

#ifndef EM_MSP430
#define EM_MSP430 	105 	/* Texas Instruments embedded microcontroller msp430 */
#endif//EM_MSP430


/*
 * e_type
 */
#ifndef ET_NONE
#define ET_NONE		0
#endif

#ifndef ET_REL
#define ET_REL		1
#endif

#ifndef ET_EXEC
#define ET_EXEC		2
#endif

#ifndef ET_DYN
#define ET_DYN		3
#endif

#ifndef ET_CORE
#define ET_CORE		4
#endif

#ifndef ET_NUM
#define ET_NUM		5
#endif

#ifndef ET_LOOS
#define ET_LOOS		0xfe00
#endif

#ifndef ET_HIOS
#define ET_HIOS		0xfeff
#endif

#ifndef ET_LOPROC
#define ET_LOPROC	0xff00
#endif

#ifndef ET_HIPROC
#define ET_HIPROC	0xffff
#endif


#endif//_SOSELF_H_
