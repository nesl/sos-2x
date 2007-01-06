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

#ifndef _KERTABLE_PROC_H_
#define _KERTABLE_PROC_H_

#include <kertable.h> // for SYS_KERTABLE_END

// NOTE - If you add a function to the PROC_KERTABLE, make sure to change
// PROC_KERTABLE_LEN
#define PROC_KER_TABLE                                          \
    /*  1 */ (void*)ker_adc_proc_bindPort,				\
    /*  2 */ (void*)ker_adc_proc_getData,				\
    /*  3 */ (void*)ker_adc_proc_getContinuousData,			\
    /*  4 */ (void*)ker_adc_proc_getCalData,				\
    /*  5 */ (void*)ker_adc_proc_getCalContinuousData,		\
    /*  6 */ 0,			\
    /*  7 */ 0,			\
    /*  8 */ 0,				\
    /*  9 */ 0,				\
    /* 10 */ (void*)ker_uart_reserve_bus,			\
    /* 11 */ (void*)ker_uart_release_bus,			\
    /* 12 */ (void*)ker_uart_send_data,				\
    /* 13 */ (void*)ker_uart_read_data,				\
    /* 14 */ 0,
    
// NOTE - Make sure to change the length if you add new functions to the
// PROC_KERTABLE
#define PROC_KERTABLE_LEN 14

#define PROC_KERTABLE_END (SYS_KERTABLE_END+PROC_KERTABLE_LEN)

#if 0
    /*  6 */ (void*)ker_i2c_reserve_bus,			\
    /*  7 */ (void*)ker_i2c_release_bus,			\
    /*  8 */ (void*)ker_i2c_send_data,				\
    /*  9 */ (void*)ker_i2c_read_data,				
    /* 14 */ (void*)ker_memmap_perms_check,			
#endif

#endif

