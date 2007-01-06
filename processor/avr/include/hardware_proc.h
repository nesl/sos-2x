/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#ifndef _HARDWARE_PROC_H_
#define _HARDWARE_PROC_H_

#include <avr/io.h>
#include <avr/interrupt.h>
//#define USE_OBSOLETE_LIBC
#ifdef USE_OBSOLETE_LIBC
#include <avr/signal.h>
#endif
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <sos_inttypes.h>

#ifdef SOS_SFI
#include <memmap.h>
//register uint16_t sfiresrv asm("r2");
#endif//SOS_SFI
//#include <flash.h>

/**
 * @brief macro definition
 */
#define NOINIT_VAR   __attribute__ ((section (".noinit")))

/**
 * @brief undefine debug functions
 */
#define DEBUG(arg...)
#define DEBUG_PID(pid,arg...)
#define DEBUG_SHORT(arg...)
#define msg_header_out(a, b)


/**
 * @brief put hardware to idle mode
 */
#define hardware_sleep()  {         \
	MCUCR |= (1 <<(SE));            \
	asm volatile ("sleep");         \
	asm volatile ("nop");           \
	asm volatile ("nop");           \
}

/**
 * @brief put hardware to sleep and ensure no interrrupt is handled
 * from page 15 of Atmega128 doc
 * When using the SEI instruction to enable interrupts, 
 * the instruction following SEI will be
 * executed before any pending interrupts.
 * That is, when sei is followed by sleep, there will be no interrupt handling.
 */
#define atomic_hardware_sleep() {   \
	MCUCR |= (1 <<(SE));            \
	asm volatile ("sei");           \
	asm volatile ("sleep");         \
	asm volatile ("nop");           \
	asm volatile ("nop");           \
}

/**
 * @brief interrupt helpers
 */
#define ENABLE_GLOBAL_INTERRUPTS()         sei()
#define DISABLE_GLOBAL_INTERRUPTS()        cli()

#define HAS_CRITICAL_SECTION       register uint8_t _prev_

#ifndef SOS_CRITICAL_SECTION_PROFILING 
#define ENTER_CRITICAL_SECTION()  \
asm volatile ( \
	"in %0, __SREG__"   "\n\t" \
	"cli"               "\n\t" \
	: "=r" (_prev_) \
	: )

#define LEAVE_CRITICAL_SECTION() \
asm volatile ( \
	"out __SREG__, %0"   "\n\t" \
	: \
	: "r" (_prev_) )
#else 
// The following implementation uses two LEDs to profile the critical section
#define ENTER_CRITICAL_SECTION()  \
do {                              \
	_prev_ = SREG;                \
	cli();                        \
	if((_prev_ & 0x80) == 0x80) PORTA ^= (1 << 1);    \
} while(0)

/*
#define LEAVE_CRITICAL_SECTION()  \
do {                              \
	if((_prev_ & 0x80) == 0x80) PORTA ^= (1 << 2);    \
	SREG = _prev_;                \
} while(0)
*/
#define LEAVE_CRITICAL_SECTION()  \
	do { if((_prev_ & 0x80) == 0x80) asm volatile("sei"); } while(0)
#endif

/**
 * @brief watchdog
 */
#define watchdog_reset() __asm__ __volatile__ ("wdr")

typedef uint16_t mod_header_ptr;

typedef uint16_t func_cb_ptr;

#define SOS_MODULE_HEADER  __attribute__ ((__progmem__))

/**
 * @brief get module header's address from program memory
 */
#define sos_get_header_address(x) ((uint16_t)(((uint32_t)&x) >> 1))

/**
 * @brief get the address of a member in module header
 */
#define sos_get_header_member(header, offset)  \
	((uint16_t)((((uint32_t)(header) << 1) + (offset)) >> 1))

/**
 * read header macros take two parameters
 * @param addr address to the header, this can be mod_header_t or func_cb_t
 * @param offset the offset to the location of parameter.  use C offset macro
 */
#define sos_read_header_byte(addr, offset) \
	pgm_read_byte_far((((uint32_t)(addr)) << 1) + (offset))
#define sos_read_header_word(addr, offset) \
	pgm_read_word_far((((uint32_t)(addr)) << 1) + (offset))
#define sos_read_header_ptr(addr, offset) \
	pgm_read_word_far((((uint32_t)(addr)) << 1) + (offset))


#endif // _HARDWARE_PROC_H_

