/**
 * \file proc_minielf.h
 * \brief Generic Processor-Specific Include File. Change this file to add new architectures.
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#if defined(AVR_MCU) || defined(EMU_MICA2)
#ifndef PACK_STRUCT
#define PACK_STRUCT __attribute__((packed))
#endif
#include <avr_minielf.h>
#elif defined(MSP430_MCU)
#ifndef PACK_STRUCT
#define PACK_STRUCT 
#endif
#include <msp430_minielf.h>
#else
#error No architecture specified
#endif
