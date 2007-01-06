/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
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
 */
/** 
 * @brief Header file for modules
 * 
 * This is the only file that module writer should include.
 * Currently, we do not support PC emulation on this.
 */

#ifndef _MODULE_H
#define _MODULE_H

#ifndef _MODULE_
#include <sos.h>
#include <sos_sched.h>
// Ram - I am including the following file here because the configuration
// needs to know what sensor board it is being compiled to
#include <sensor.h>
#include <sos_error_types.h>
#include <random.h>
#endif

#include <sos_info.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <monitor.h>
#include <message_types.h>
#include <codemem.h>
#include <systime.h>
#include <sos_cam.h>
#include <pid.h>
#include <stddef.h>
#include <kertable_conf.h>
#include <fntable_types.h>
#include <sos_error_types.h>
#ifdef SOS_SFI
#include <memmap.h>
#endif
// Ram - I have removed the following include file
// Module writers interacting with sensorboards
// should specify the header file for that board
// explicitly in their files
//#include <sensor.h>

#ifdef _MODULE_
#include <module_proc.h>
#include <module_plat.h>

/* 10 */ uint16_t ker_hw_type();

/* 11 */ uint16_t ker_id();

/* 12 */ uint16_t ker_rand();

/* 13 */ uint32_t ker_systime32();

// sos_blk_mem_alloc
/* 14 */ void dummy14();

// sos_blk_mem_free
/* 15 */ void dummy15();

// sos_blk_mem_realloc
/* 16 */ void dummy16();

// sos_blk_mem_change_own
/* 17 */ void dummy17();

/* 18 */ uint8_t* ker_msg_take_data(sos_pid_t pid, Message *msg);

/* 19 */ int8_t ker_msg_change_rules(sos_pid_t sid, uint8_t rules);

/* 20 */  int8_t ker_timer_init(sos_pid_t pid, uint8_t tid, uint8_t type);

/* 21 */ int8_t ker_timer_start(sos_pid_t pid, uint8_t tid, int32_t interval);

/* 22 */ int8_t ker_timer_restart(sos_pid_t pid, uint8_t tid, int32_t interval) ;

/* 23 */ int8_t ker_timer_stop(sos_pid_t pid, uint8_t tid) ;

/* 24 */ int8_t ker_timer_release(sos_pid_t pid, uint8_t tid) ;

/* 25 */ int8_t post_link(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr);

/* 26 */ int8_t post(Message *m);

/* 27 */ int8_t post_short(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t byte, uint16_t word, uint16_t flag);

/* 28 */ int8_t post_long(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag);

/* 29 */ int8_t post_longer(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t saddr);

/* 30 */ node_loc_t ker_loc();

/* 31 */ gps_t ker_gps();

/* 32 */ uint32_t ker_loc_r2(node_loc_t *loc1, node_loc_t *loc2);

/* 33 */ uint16_t ker_systime16L();

/* 34 */ uint16_t ker_systime16H();

/* 35 */ int8_t ker_register_module(mod_header_ptr h) ;

/* 36 */ int8_t ker_deregister_module(sos_pid_t pid) ;

/* 37 */ sos_module_t* ker_get_module(sos_pid_t pid) ;

/* 38 */ int8_t ker_register_monitor(sos_pid_t pid, uint8_t type, monitor_cb *cb);

/* 39 */ int8_t ker_deregister_monitor(monitor_cb *cb);

/* 40 */ int8_t ker_fntable_subscribe(sos_pid_t sub_pid, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index);

/* 41 */ int8_t ker_sensor_register(sos_pid_t sensor_driver_pid, uint8_t sensor_type, uint8_t get_data_fid);

/* 42 */ int8_t ker_sensor_deregister(sos_pid_t sensor_driver_pid, uint8_t sensor_type, uint8_t get_data_fid);

/* 43 */ int8_t ker_sensor_get_data(sos_pid_t client_pid, uint8_t sensor_type);

/* 44 */ int8_t ker_sensor_data_ready(uint8_t sensor_type, uint16_t sensor_data);

/* 45 */ sos_pid_t ker_set_current_pid( sos_pid_t pid );

/* 46 */ sos_pid_t ker_get_current_pid( void );

/* 47 */ int8_t ker_sensor_data_fail(uint8_t sensor_type);

/* 48 */ void* ker_get_module_state(sos_pid_t pid);

/* 49 */ sos_pid_t  ker_spawn_module(mod_header_ptr h,
		        void *init, uint8_t init_size, uint8_t flag);

/* 50 */ mod_header_ptr ker_codemem_get_header_from_code_id( sos_code_id_t cid );

/* 51 */ mod_header_ptr ker_codemem_get_header_address(codemem_t cid);

/* 52 */ codemem_t ker_codemem_alloc(uint16_t size, codemem_type_t type);
/* 53 */ int8_t ker_codemem_write(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset);

/* 54 */ int8_t ker_codemem_read(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset);

/* 55 */ int8_t ker_codemem_free(codemem_t h);

/* 56 */ int8_t ker_codemem_flush(codemem_t h, sos_pid_t pid);

/* 57 */ dummy_func ker_get_func_ptr(func_cb_ptr p, sos_pid_t *prev);
	
/* 58 */ int8_t ker_panic(void);

/* 59 */ int8_t ker_mod_panic(sos_pid_t pid);


#include <module_virtual.h>

/**
 * @brief critical section helpers
 */
#include <hardware_types.h>

#endif /* #ifdef _MODULE_ */
#endif /* #ifndef _MODULE_H */

