/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
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

#ifndef _HARDWARE_PROC_H_
#define _HARDWARE_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ML674000.h>
#include <irq.h>
#include <wdt.h>
//#include <progmem.h>

/*****************************************************/
/*    internal I/O input/output macro                */
/*****************************************************/
#define get_value(n)    (*((volatile uint8_t *)(n)))          /* byte input */
#define put_value(n,c)  (*((volatile uint8_t *)(n)) = (c))    /* byte output */
#define get_hvalue(n)   (*((volatile uint16_t *)(n)))         /* half word input */
#define put_hvalue(n,c) (*((volatile uint16_t *)(n)) = (c))   /* half word output */
#define get_wvalue(n)   (*((volatile uint32_t *)(n)))          /* word input */
#define put_wvalue(n,c) (*((volatile uint32_t *)(n)) = (c))    /* word output */
#define set_bit(n,c)    (*((volatile uint8_t *)(n))|= (c))    /* byte bit set */
#define clr_bit(n,c)    (*((volatile uint8_t *)(n))&=~(c))    /* byte bit clear */
#define set_hbit(n,c)   (*((volatile uint16_t *)(n))|= (c))   /* half word bit set */
#define clr_hbit(n,c)   (*((volatile uint16_t *)(n))&=~(c))   /* half word bit clear */
#define set_wbit(n,c)   (*((volatile uint32_t *)(n))|= (c))    /* word bit set */
#define clr_wbit(n,c)   (*((volatile uint32_t *)(n))&=~(c))    /* word bit clear */

/** Read Data **/
#define in_b(n)		(*((volatile uint8_t *)(n)))
#define in_hw(n)	(*((volatile uint16_t *)(n)))
#define in_w(n)		(*((volatile uint32_t *)(n)))

/** Write Data **/
#define out_b(n,c)	(*((volatile uint8_t *)(n)) = (c))
#define out_hw(n,c)	(*((volatile uint16_t *)(n)) = (c))
#define out_w(n,c)	(*((volatile uint32_t *)(n)) = (c))

#ifndef SemiSWI
#ifdef __thumb
/* Define Angel Semihosting SWI to be Thumb one */
#define SemiSWI (0xAB)
#else
/* Define Angel Semihosting SWI to be ARM one */
#define SemiSWI (0x123456)
#endif
#endif

#define NOINIT_VAR

/**
 * @brief undefine debug functions
 */
#define DEBUG(arg...)
#define DEBUG_PID(pid,arg...)
#define DEBUG_SHORT(arg...)
#define msg_header_out(a, b)

#define hardware_sleep()
#define atomic_hardware_sleep() { watchdog_reset(); ENABLE_GLOBAL_INTERRUPTS(); }

/**
 * @brief interrupt helpers
 */
#define HAS_CRITICAL_SECTION       int _prev_
#define ENTER_CRITICAL_SECTION()   { _prev_ = !get_irq_state(); irq_dis(); }
#define LEAVE_CRITICAL_SECTION()   { if(_prev_) irq_en(); }
#define DISABLE_GLOBAL_INTERRUPTS()         irq_en()
#define ENABLE_GLOBAL_INTERRUPTS()        irq_dis()

/**
 * @brief watchdog
 */
#define watchdog_reset()	wdt_reset()

typedef uint32_t mod_header_ptr;

typedef uint32_t func_cb_ptr;

//#define SOS_MODULE_HEADER PROGMEM
#define SOS_MODULE_HEADER __attribute__((section(".progmem")))

#define sos_get_header_address(x) ((uint32_t)&(x))

#define sos_get_header_member(header, offset) ((uint32_t)((header)+(offset)))

#define sos_read_header_byte(addr, offset) (*(uint8_t*)(addr + offset))
#define sos_read_header_word(addr, offset) (*(uint16_t*)(addr + offset))
#define sos_read_header_ptr(addr, offset)  (*(void**)(addr + offset))


/**
 * @brief cache control
 */
#define CACHE_BASE	(0x78200000)				// should these really go here?
#define CON       	(CACHE_BASE+0x04)
#define CACHE       (CACHE_BASE+0x08)
#define FLUSH       (CACHE_BASE+0x1C)

#define ENABLE_CACHE()	do{put_wvalue(CACHE, 0x00000000);\
						   put_wvalue(CON,   0x00000000);\
						   put_wvalue(FLUSH, 0x00000001);\
						   put_wvalue(CACHE, 0x00010000);}while(0);		//enable bank 0

#define DISABLE_CACHE()	do{put_wvalue(CACHE, 0x00000000);}while(0);

/**
 * @brief clock parameters
 */
#define MHz     (1000000L)
#define CCLK    (56*MHz)    /* frequency of CCLK */

#define MILI_SEC_10  ( 1 * ((CCLK / (32 * 1000)) ))  /* 1ms */
#define NANO_SEC_30  ( 30 * ((CCLK / (32 * 1000)) ))  /* 30us */

#define BAUDRATE    (38400UL)      /* baudrate */
#define BL_DIV      ((CCLK+(8*BAUDRATE))/(16*BAUDRATE))  /* 0x12 */

#define VALUE_OF_DLL	((uint8_t)(BL_DIV & 0xff))            /* bit0-bit7 of BL_DIV */
#define VALUE_OF_DLM	((uint8_t)((BL_DIV & 0xff00) >> 8))   /* bit8-bit15 of BL_DIV */

#ifdef __cplusplus
};      /* end of 'extern "C"' */
#endif



#endif  /* end of uPLAT.h */
