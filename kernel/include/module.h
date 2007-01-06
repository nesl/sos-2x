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

/* 10 */  
typedef uint16_t (* ker_hw_type_ker_func_t)( void);
static inline uint16_t ker_hw_type( void)
{
	ker_hw_type_ker_func_t func = 
		(ker_hw_type_ker_func_t)get_kertable_entry(10);
	return func( );
}


/* 11 */  
typedef uint16_t (* ker_id_ker_func_t)( void);
static inline uint16_t ker_id( void)
{
	ker_id_ker_func_t func = 
		(ker_id_ker_func_t)get_kertable_entry(11);
	return func( );
}


/* 12 */  
typedef uint16_t (* ker_rand_ker_func_t)( void);
static inline uint16_t ker_rand( void)
{
	ker_rand_ker_func_t func = 
		(ker_rand_ker_func_t)get_kertable_entry(12);
	return func( );
}


/* 13 */  
typedef uint32_t (* ker_systime32_ker_func_t)( void);
static inline uint32_t ker_systime32( void)
{
	ker_systime32_ker_func_t func = 
		(ker_systime32_ker_func_t)get_kertable_entry(13);
	return func( );
}


// sos_blk_mem_alloc
/* 14 */  
typedef void (* dummy14_ker_func_t)( void);
static inline void dummy14( void)
{
	dummy14_ker_func_t func = 
		(dummy14_ker_func_t)get_kertable_entry(14);
	func( );
}


// sos_blk_mem_free
/* 15 */  
typedef void (* dummy15_ker_func_t)( void);
static inline void dummy15( void)
{
	dummy15_ker_func_t func = 
		(dummy15_ker_func_t)get_kertable_entry(15);
	func( );
}


// sos_blk_mem_realloc
/* 16 */  
typedef void (* dummy16_ker_func_t)( void);
static inline void dummy16( void)
{
	dummy16_ker_func_t func = 
		(dummy16_ker_func_t)get_kertable_entry(16);
	func( );
}


// sos_blk_mem_change_own
/* 17 */  
typedef void (* dummy17_ker_func_t)( void);
static inline void dummy17( void)
{
	dummy17_ker_func_t func = 
		(dummy17_ker_func_t)get_kertable_entry(17);
	func( );
}


/* 18 */     
typedef uint8_t *  (* ker_msg_take_data_ker_func_t)( sos_pid_t pid, Message *  msg );
static inline uint8_t *  ker_msg_take_data( sos_pid_t pid, Message *  msg )
{
	ker_msg_take_data_ker_func_t func = 
		(ker_msg_take_data_ker_func_t)get_kertable_entry(18);
	return func( pid, msg );
}


/* 19 */     
typedef int8_t (* ker_msg_change_rules_ker_func_t)( sos_pid_t sid, uint8_t rules );
static inline int8_t ker_msg_change_rules( sos_pid_t sid, uint8_t rules )
{
	ker_msg_change_rules_ker_func_t func = 
		(ker_msg_change_rules_ker_func_t)get_kertable_entry(19);
	return func( sid, rules );
}


/* 20 */        
typedef int8_t (* ker_timer_init_ker_func_t)( sos_pid_t pid, uint8_t tid, uint8_t type );
static inline int8_t ker_timer_init( sos_pid_t pid, uint8_t tid, uint8_t type )
{
	ker_timer_init_ker_func_t func = 
		(ker_timer_init_ker_func_t)get_kertable_entry(20);
	return func( pid, tid, type );
}


/* 21 */       
typedef int8_t (* ker_timer_start_ker_func_t)( sos_pid_t pid, uint8_t tid, int32_t interval );
static inline int8_t ker_timer_start( sos_pid_t pid, uint8_t tid, int32_t interval )
{
	ker_timer_start_ker_func_t func = 
		(ker_timer_start_ker_func_t)get_kertable_entry(21);
	return func( pid, tid, interval );
}


/* 22 */        
typedef int8_t (* ker_timer_restart_ker_func_t)( sos_pid_t pid, uint8_t tid, int32_t interval );
static inline int8_t ker_timer_restart( sos_pid_t pid, uint8_t tid, int32_t interval )
{
	ker_timer_restart_ker_func_t func = 
		(ker_timer_restart_ker_func_t)get_kertable_entry(22);
	return func( pid, tid, interval );
}


/* 23 */      
typedef int8_t (* ker_timer_stop_ker_func_t)( sos_pid_t pid, uint8_t tid );
static inline int8_t ker_timer_stop( sos_pid_t pid, uint8_t tid )
{
	ker_timer_stop_ker_func_t func = 
		(ker_timer_stop_ker_func_t)get_kertable_entry(23);
	return func( pid, tid );
}


/* 24 */      
typedef int8_t (* ker_timer_release_ker_func_t)( sos_pid_t pid, uint8_t tid );
static inline int8_t ker_timer_release( sos_pid_t pid, uint8_t tid )
{
	ker_timer_release_ker_func_t func = 
		(ker_timer_release_ker_func_t)get_kertable_entry(24);
	return func( pid, tid );
}


/* 25 */               
typedef int8_t (* post_link_ker_func_t)( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag, uint16_t daddr );
static inline int8_t post_link( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag, uint16_t daddr )
{
	post_link_ker_func_t func = 
		(post_link_ker_func_t)get_kertable_entry(25);
	return func( did, sid, type, arg, larg, flag, daddr );
}


/* 26 */   
typedef int8_t (* post_ker_func_t)( Message *  m );
static inline int8_t post( Message *  m )
{
	post_ker_func_t func = 
		(post_ker_func_t)get_kertable_entry(26);
	return func( m );
}


/* 27 */             
typedef int8_t (* post_short_ker_func_t)( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t byte, uint16_t word, uint16_t flag );
static inline int8_t post_short( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t byte, uint16_t word, uint16_t flag )
{
	post_short_ker_func_t func = 
		(post_short_ker_func_t)get_kertable_entry(27);
	return func( did, sid, type, byte, word, flag );
}


/* 28 */             
typedef int8_t (* post_long_ker_func_t)( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag );
static inline int8_t post_long( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag )
{
	post_long_ker_func_t func = 
		(post_long_ker_func_t)get_kertable_entry(28);
	return func( did, sid, type, arg, larg, flag );
}


/* 29 */               
typedef int8_t (* post_longer_ker_func_t)( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag, uint16_t saddr );
static inline int8_t post_longer( sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag, uint16_t saddr )
{
	post_longer_ker_func_t func = 
		(post_longer_ker_func_t)get_kertable_entry(29);
	return func( did, sid, type, arg, larg, flag, saddr );
}


/* 30 */  
typedef node_loc_t (* ker_loc_ker_func_t)( void);
static inline node_loc_t ker_loc( void)
{
	ker_loc_ker_func_t func = 
		(ker_loc_ker_func_t)get_kertable_entry(30);
	return func( );
}


/* 31 */  
typedef gps_t (* ker_gps_ker_func_t)( void);
static inline gps_t ker_gps( void)
{
	ker_gps_ker_func_t func = 
		(ker_gps_ker_func_t)get_kertable_entry(31);
	return func( );
}


/* 32 */     
typedef uint32_t (* ker_loc_r2_ker_func_t)( node_loc_t *  loc1, node_loc_t *  loc2 );
static inline uint32_t ker_loc_r2( node_loc_t *  loc1, node_loc_t *  loc2 )
{
	ker_loc_r2_ker_func_t func = 
		(ker_loc_r2_ker_func_t)get_kertable_entry(32);
	return func( loc1, loc2 );
}


/* 33 */  
typedef uint16_t (* ker_systime16L_ker_func_t)( void);
static inline uint16_t ker_systime16L( void)
{
	ker_systime16L_ker_func_t func = 
		(ker_systime16L_ker_func_t)get_kertable_entry(33);
	return func( );
}


/* 34 */  
typedef uint16_t (* ker_systime16H_ker_func_t)( void);
static inline uint16_t ker_systime16H( void)
{
	ker_systime16H_ker_func_t func = 
		(ker_systime16H_ker_func_t)get_kertable_entry(34);
	return func( );
}


/* 35 */    
typedef int8_t (* ker_register_module_ker_func_t)( mod_header_ptr h );
static inline int8_t ker_register_module( mod_header_ptr h )
{
	ker_register_module_ker_func_t func = 
		(ker_register_module_ker_func_t)get_kertable_entry(35);
	return func( h );
}


/* 36 */    
typedef int8_t (* ker_deregister_module_ker_func_t)( sos_pid_t pid );
static inline int8_t ker_deregister_module( sos_pid_t pid )
{
	ker_deregister_module_ker_func_t func = 
		(ker_deregister_module_ker_func_t)get_kertable_entry(36);
	return func( pid );
}


/* 37 */    
typedef sos_module_t *  (* ker_get_module_ker_func_t)( sos_pid_t pid );
static inline sos_module_t *  ker_get_module( sos_pid_t pid )
{
	ker_get_module_ker_func_t func = 
		(ker_get_module_ker_func_t)get_kertable_entry(37);
	return func( pid );
}


/* 38 */       
typedef int8_t (* ker_register_monitor_ker_func_t)( sos_pid_t pid, uint8_t type, monitor_cb *  cb );
static inline int8_t ker_register_monitor( sos_pid_t pid, uint8_t type, monitor_cb *  cb )
{
	ker_register_monitor_ker_func_t func = 
		(ker_register_monitor_ker_func_t)get_kertable_entry(38);
	return func( pid, type, cb );
}


/* 39 */   
typedef int8_t (* ker_deregister_monitor_ker_func_t)( monitor_cb *  cb );
static inline int8_t ker_deregister_monitor( monitor_cb *  cb )
{
	ker_deregister_monitor_ker_func_t func = 
		(ker_deregister_monitor_ker_func_t)get_kertable_entry(39);
	return func( cb );
}


/* 40 */         
typedef int8_t (* ker_fntable_subscribe_ker_func_t)( sos_pid_t sub_pid, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index );
static inline int8_t ker_fntable_subscribe( sos_pid_t sub_pid, sos_pid_t pub_pid, uint8_t fid, uint8_t table_index )
{
	ker_fntable_subscribe_ker_func_t func = 
		(ker_fntable_subscribe_ker_func_t)get_kertable_entry(40);
	return func( sub_pid, pub_pid, fid, table_index );
}


/* 41 */       
typedef int8_t (* ker_sensor_register_ker_func_t)( sos_pid_t sensor_driver_pid, uint8_t sensor_id, uint8_t sensor_fid, void* data );
static inline int8_t ker_sensor_register( sos_pid_t sensor_driver_pid, uint8_t sensor_id, uint8_t sensor_fid, void* data )
{
	ker_sensor_register_ker_func_t func = 
		(ker_sensor_register_ker_func_t)get_kertable_entry(41);
	return func( sensor_driver_pid, sensor_id, sensor_fid, data );
}


/* 42 */       
typedef int8_t (* ker_sensor_deregister_ker_func_t)( sos_pid_t sensor_driver_pid, uint8_t sensor_id );
static inline int8_t ker_sensor_deregister( sos_pid_t sensor_driver_pid, uint8_t sensor_id )
{
	ker_sensor_deregister_ker_func_t func = 
		(ker_sensor_deregister_ker_func_t)get_kertable_entry(42);
	return func( sensor_driver_pid, sensor_id );
}


/* 43 */     
typedef int8_t (* ker_sensor_get_data_ker_func_t)( sos_pid_t client_pid, uint8_t sensor_id );
static inline int8_t ker_sensor_get_data( sos_pid_t client_pid, uint8_t sensor_id )
{
	ker_sensor_get_data_ker_func_t func = 
		(ker_sensor_get_data_ker_func_t)get_kertable_entry(43);
	return func( client_pid, sensor_id );
}


/* 44 */     
typedef int8_t (* ker_sensor_data_ready_ker_func_t)( uint8_t sensor_id, uint16_t sensor_data, uint8_t status );
static inline int8_t ker_sensor_data_ready( uint8_t sensor_id, uint16_t sensor_data, uint8_t status )
{
	ker_sensor_data_ready_ker_func_t func = 
		(ker_sensor_data_ready_ker_func_t)get_kertable_entry(44);
	return func( sensor_id, sensor_data, status );
}

/* 45 */     
typedef int8_t (* ker_sensor_enable_ker_func_t)( sos_pid_t client_pid, uint8_t sensor_id );
static inline int8_t ker_sensor_enable( sos_pid_t client_pid, uint8_t sensor_id )
{
	ker_sensor_enable_ker_func_t func = 
		(ker_sensor_enable_ker_func_t)get_kertable_entry(45);
	return func( client_pid, sensor_id );
}

/* 46 */     
static inline int8_t ker_sensor_disable( sos_pid_t client_pid, uint8_t sensor_id )
{
	ker_sensor_enable_ker_func_t func = 
		(ker_sensor_enable_ker_func_t)get_kertable_entry(46);
	return func( client_pid, sensor_id );
}

/* 47 */     
typedef int8_t (* ker_sensor_control_ker_func_t)( sos_pid_t client_pid, uint8_t sensor_id, void* sensor_new_state );
static inline int8_t ker_sensor_control( sos_pid_t client_pid, uint8_t sensor_id, void* sensor_new_state )
{
	ker_sensor_control_ker_func_t func = 
		(ker_sensor_control_ker_func_t)get_kertable_entry(47);
	return func( client_pid, sensor_id, sensor_new_state );
}



/* 48 */     
typedef sos_pid_t (* ker_set_current_pid_ker_func_t)( sos_pid_t pid );
static inline sos_pid_t ker_set_current_pid( sos_pid_t pid )
{
	ker_set_current_pid_ker_func_t func = 
		(ker_set_current_pid_ker_func_t)get_kertable_entry(48);
	return func( pid );
}


/* 49 */    
typedef sos_pid_t (* ker_get_current_pid_ker_func_t)( void);
static inline sos_pid_t ker_get_current_pid( void)
{
	ker_get_current_pid_ker_func_t func = 
		(ker_get_current_pid_ker_func_t)get_kertable_entry(49);
	return func( );
}


/* 50    
typedef int8_t (* ker_sensor_data_fail_ker_func_t)( uint8_t sensor_type );
static inline int8_t ker_sensor_data_fail( uint8_t sensor_type )
{
	ker_sensor_data_fail_ker_func_t func = 
		(ker_sensor_data_fail_ker_func_t)get_kertable_entry(50);
	return func( sensor_type );
}
*/

/* 50 */   
typedef void *  (* ker_get_module_state_ker_func_t)( sos_pid_t pid );
static inline void *  ker_get_module_state( sos_pid_t pid )
{
	ker_get_module_state_ker_func_t func = 
		(ker_get_module_state_ker_func_t)get_kertable_entry(50);
	return func( pid );
}


/* 51 */    
		             
typedef sos_pid_t (* ker_spawn_module_ker_func_t)( mod_header_ptr h, void *  init, uint8_t init_size, uint8_t flag );
static inline sos_pid_t ker_spawn_module( mod_header_ptr h, void *  init, uint8_t init_size, uint8_t flag )
{
	ker_spawn_module_ker_func_t func = 
		(ker_spawn_module_ker_func_t)get_kertable_entry(51);
	return func( h, init, init_size, flag );
}


/* 52 */     
typedef mod_header_ptr (* ker_codemem_get_header_from_code_id_ker_func_t)( sos_code_id_t cid );
static inline mod_header_ptr ker_codemem_get_header_from_code_id( sos_code_id_t cid )
{
	ker_codemem_get_header_from_code_id_ker_func_t func = 
		(ker_codemem_get_header_from_code_id_ker_func_t)get_kertable_entry(52);
	return func( cid );
}


/* 53 */   
typedef mod_header_ptr (* ker_codemem_get_header_address_ker_func_t)( codemem_t cid );
static inline mod_header_ptr ker_codemem_get_header_address( codemem_t cid )
{
	ker_codemem_get_header_address_ker_func_t func = 
		(ker_codemem_get_header_address_ker_func_t)get_kertable_entry(53);
	return func( cid );
}


/* 54 */     
typedef codemem_t (* ker_codemem_alloc_ker_func_t)( uint16_t size, codemem_type_t type );
static inline codemem_t ker_codemem_alloc( uint16_t size, codemem_type_t type )
{
	ker_codemem_alloc_ker_func_t func = 
		(ker_codemem_alloc_ker_func_t)get_kertable_entry(54);
	return func( size, type );
}

/* 55 */           
typedef int8_t (* ker_codemem_write_ker_func_t)( codemem_t h, sos_pid_t pid, void *  buf, uint16_t nbytes, uint16_t offset );
static inline int8_t ker_codemem_write( codemem_t h, sos_pid_t pid, void *  buf, uint16_t nbytes, uint16_t offset )
{
	ker_codemem_write_ker_func_t func = 
		(ker_codemem_write_ker_func_t)get_kertable_entry(55);
	return func( h, pid, buf, nbytes, offset );
}


/* 56 */           
typedef int8_t (* ker_codemem_read_ker_func_t)( codemem_t h, sos_pid_t pid, void *  buf, uint16_t nbytes, uint16_t offset );
static inline int8_t ker_codemem_read( codemem_t h, sos_pid_t pid, void *  buf, uint16_t nbytes, uint16_t offset )
{
	ker_codemem_read_ker_func_t func = 
		(ker_codemem_read_ker_func_t)get_kertable_entry(56);
	return func( h, pid, buf, nbytes, offset );
}


/* 57 */   
typedef int8_t (* ker_codemem_free_ker_func_t)( codemem_t h );
static inline int8_t ker_codemem_free( codemem_t h )
{
	ker_codemem_free_ker_func_t func = 
		(ker_codemem_free_ker_func_t)get_kertable_entry(57);
	return func( h );
}


/* 58 */     
typedef int8_t (* ker_codemem_flush_ker_func_t)( codemem_t h, sos_pid_t pid );
static inline int8_t ker_codemem_flush( codemem_t h, sos_pid_t pid )
{
	ker_codemem_flush_ker_func_t func = 
		(ker_codemem_flush_ker_func_t)get_kertable_entry(58);
	return func( h, pid );
}


/* 59 */     
typedef dummy_func (* ker_get_func_ptr_ker_func_t)( func_cb_ptr p, sos_pid_t *  prev );
static inline dummy_func ker_get_func_ptr( func_cb_ptr p, sos_pid_t *  prev )
{
	ker_get_func_ptr_ker_func_t func = 
		(ker_get_func_ptr_ker_func_t)get_kertable_entry(59);
	return func( p, prev );
}

	
/* 60 */  
typedef int8_t (* ker_panic_ker_func_t)( void);
static inline int8_t ker_panic( void)
{
	ker_panic_ker_func_t func = 
		(ker_panic_ker_func_t)get_kertable_entry(60);
	return func( );
}


/* 61 */   
typedef int8_t (* ker_mod_panic_ker_func_t)( sos_pid_t pid );
static inline int8_t ker_mod_panic( sos_pid_t pid )
{
	ker_mod_panic_ker_func_t func = 
		(ker_mod_panic_ker_func_t)get_kertable_entry(61);
	return func( pid );
}



#include <module_virtual.h>

/**
 * @brief critical section helpers
 */
#include <hardware_types.h>

#endif /* #ifdef _MODULE_ */
#endif /* #ifndef _MODULE_H */

