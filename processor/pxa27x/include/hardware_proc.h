/*
 * Copyright (c) 2005 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * @author  Andrew Barton-Sweeney   (abs@cs.yale.edu)
 *
 */

#ifndef _HARDWARE_PROC_H_
#define _HARDWARE_PROC_H_

#include <sos_inttypes.h>
#include <pxa27x_registers.h>
#include <pxa27xhardware.h>
#include <pgmspace.h>

/**
 * @brief macro definition
 */
#define NOINIT_VAR   //__attribute__ ((section (".noinit")))

/**
 *  Debug Output
 */
#define DEBUG(arg...)
#define DEBUG_PID(pid,arg...)
#define DEBUG_SHORT(arg...)
#define msg_header_out(a, b)

/**
 *  Hardware Sleep Control
 */
#define hardware_sleep()
#define atomic_hardware_sleep()			local_irq_enable()

/**
 *  Interrupt Control
 */
#define ENABLE_GLOBAL_INTERRUPTS()		local_irq_enable()
#define DISABLE_GLOBAL_INTERRUPTS()		local_irq_disable()

#define HAS_CRITICAL_SECTION			__local_atomic_t oldState
#define ENTER_CRITICAL_SECTION()		oldState = local_atomic_start()
#define LEAVE_CRITICAL_SECTION()		local_atomic_end(oldState)

/**
 *  Watchdog Reset
 */
#define watchdog_reset()

/**
 *  Module and Function Pointers
 */
typedef uint32_t mod_header_ptr;
typedef uint32_t func_cb_ptr;

/**
 *  Read Module Header
 */
//#define SOS_MODULE_HEADER PROGMEM
#define SOS_MODULE_HEADER __attribute__((section(".progmem")))

#define sos_get_header_address(x) ((uint32_t)&(x))
#define sos_get_header_member(header, offset) ((uint32_t)((header)+(offset)))
#define sos_read_header_byte(addr, offset) (*(uint8_t*)(addr + offset))
#define sos_read_header_word(addr, offset) (*(uint16_t*)(addr + offset))
#define sos_read_header_ptr(addr, offset)  (*(void**)(addr + offset))

#endif // _HARDWARE_PROC_H_
