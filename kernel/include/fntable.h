/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
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
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
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
 * @brief    Function Pointer Table
 * @author   Ram Kumar (ram@ee.ucla.edu)
 */

#ifndef _FNTABLE_H_
#define _FNTABLE_H_

#include <pid.h>
#include <stdarg.h>
#include <hardware_types.h>
#include <sos_module_types.h>
#include <sos_linker_conf.h>
#include <fntable_types.h>


#ifndef _MODULE_


/**
 * @brief Initializes the function table
 */
extern int8_t fntable_init(void);

/**
 * @brief subscribe to function pointer
 * @param sos_pid_t sub_pid Module requesting function. (User)
 * @param sos_pid_t pub_pid Module implementing the requested function. (Provider)
 * @param uint8_t fid function id the module is going to subscribe
 * @param uint8_t table_index the index to the function record, starting zero
 * @return errno
 */
extern int8_t ker_fntable_subscribe(sos_pid_t sub_pid, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index);


/**
 * @brief the notifier for kernel telling function_table that a module is removed
 * @param hdr module header that will be removed
 */
extern int8_t fntable_remove_all(sos_module_t *m);



/**
 * @brief Link to all the functions that a module subscribes to
 * @note Used for micro-rebooting the module
 */
extern void fntable_link_subscribed_functions(sos_module_t *m);


#ifdef MINIELF_LOADER
/**
 * @brief link the module into correct address
 * NOTE: this rountine assumes the header is in RAM
 *
 * @param hdr pointer to module header in RAM
 * @param base_addr base address
 */
void fntable_fix_address(        
		func_addr_t  base_addr,        
		uint8_t      num_funcs,        
		uint8_t     *buf,        
		uint16_t     nbytes,        
		func_addr_t  offset);
#endif//MINIELF_LOADER

/**
 * @brief unlink the module from absolute address into position independent code
 * NOTE: this rountine assumes the header is in RAM
 *
 * @param hdr pointer to module header in RAM
 * @param base_addr base address
 */
void fntable_unfix_address(                                   
		func_addr_t  base_addr,                               
		uint8_t      num_funcs,                               
		uint8_t     *buf,                                     
		uint16_t     nbytes,                                  
		func_addr_t  offset);   

/**
 * @brief link the functions
 */
int8_t fntable_link(sos_module_t *m);

/**
 * @brief error stub for function subscriber
 */
extern void error_v(func_cb_ptr);
extern int8_t error_8(func_cb_ptr);
extern int16_t error_16(func_cb_ptr);
extern int32_t error_32(func_cb_ptr);
extern void* error_ptr(func_cb_ptr);

extern dummy_func ker_get_func_ptr(func_cb_ptr p, sos_pid_t *prev);
//extern int8_t error_stub(func_cb_ptr proto, ...);
extern void sys_fnptr_call( func_cb_ptr p );

#endif //_MODULE_

#endif //_FNTABLE_H_
