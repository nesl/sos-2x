/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#ifndef _HARDWARE_PROC_H
#define _HARDWARE_PROC_H
#include <io.h>
#include <signal.h>
#include <msp430/flash.h>
#include <pgmspace1_16.h>

#include <sos_inttypes.h>

typedef uint16_t mod_header_ptr;
typedef uint16_t func_cb_ptr;

/**
 * @brief Macro definition
 */
#define NOINIT_VAR __attribute__ ((section (".noinit")))

/**
 * @brief Undefine debug functions
 */
#define DEBUG(arg...)
#define DEBUG_PID(pid,arg...)
#define DEBUG_SHORT(arg...)
#define msg_header_out(a, b)




/**
 * @brief Interrupt Handlers
 */
#define HAS_CRITICAL_SECTION      register uint8_t _prev_
#define ENTER_CRITICAL_SECTION()  _prev_ = READ_SR & GIE; dint()
#define LEAVE_CRITICAL_SECTION()  if(_prev_) eint()
#define ENABLE_GLOBAL_INTERRUPTS()        eint()
#define DISABLE_GLOBAL_INTERRUPTS()       dint()


/**
 * @brief Watchdog
 */
#define WDT_RESET_WORD           (WDTPW | WDTCNTCL | WDTSSEL)
//#define watchdog_reset()         WDTCTL |= WDT_RESET_WORD
#define watchdog_reset()

#define DISABLE_WDT()			  (WDTCTL = (WDTPW | WDTHOLD))

#define ENABLE_WDT()				\
  watchdog_reset()

#define SOS_MODULE_HEADER  
	//__attribute__ ((section(".text")))

#define sos_get_header_address(x) ((uint16_t)&(x))
#define sos_get_header_member(header, offset) ((header)+(offset))
#define sos_read_header_byte(addr, offset) (*(uint8_t*)(addr + offset))
#define sos_read_header_word(addr, offset) (*(uint16_t*)(addr + offset))
#define sos_read_header_ptr(addr, offset)  (*(void**)(addr + offset))

// XXX - TODO - Put the CPU to sleep before enabling interrupts
#define atomic_hardware_sleep() {eint();}

#endif

