/**
 * \mainpage [SOS System API]
 *
 * Provided here is the documentation for the system API provided by the SOS
 * kernel for user modules.
 *
 * The link below will take you back to the main SOS documentation website:
 *
 * https://projects.nesl.ucla.edu/public/sos-2x/
 */

#ifndef __SYS_MODULE_H__
#define __SYS_MODULE_H__

#ifndef _MODULE_
#include <sos.h>
#endif

#include <sos_info.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <pid.h>
#include <stddef.h>
#include <kertable_conf.h>  // for get_kertable_entry()
#include <sos_error_types.h>
#include <codemem.h>
#include <sos_shm.h>

#ifndef SYS_JUMP_TBL_START
/// \cond NOTYPEDEF
void* ker_sys_malloc(uint16_t size);
void* ker_sys_realloc(void* pntr, uint16_t newSize);
void ker_sys_free(void *pntr);
void* ker_sys_msg_take_data(Message *msg);
int8_t ker_sys_timer_start(uint8_t tid, int32_t interval, uint8_t type);
int8_t ker_sys_timer_restart(uint8_t tid, int32_t interval);
int8_t ker_sys_timer_stop(uint8_t tid);
int8_t ker_sys_post(sos_pid_t did, uint8_t type, uint8_t size, 
		    void *data, uint16_t flag);
int8_t ker_sys_post_link(sos_pid_t dst_mod_id, uint8_t type,
    uint8_t size, void *data, uint16_t flag, uint16_t dst_node_addr);
int8_t ker_sys_post_value(sos_pid_t dst_mod_id,
                uint8_t type, uint32_t data, uint16_t flag);
uint8_t ker_hw_type();
uint16_t ker_id();
uint16_t ker_rand();
uint32_t ker_systime32();
int8_t ker_sys_sensor_get_data( uint8_t sensor_id );
int8_t ker_led(uint8_t op);
void* ker_sys_get_module_state( void );
int8_t ker_sys_fntable_subscribe( sos_pid_t pub_pid, uint8_t fid, uint8_t table_index );
int8_t ker_sys_change_own( void* ptr );
int8_t ker_sys_codemem_read(codemem_t h, void *buf, uint16_t nbytes, uint16_t offset);
int8_t ker_sys_shm_open( sos_shm_t name, void *shm );
int8_t ker_sys_shm_update( sos_shm_t name, void *shm );
int8_t ker_sys_shm_close( sos_shm_t name );
void* ker_sys_shm_get( sos_shm_t name );
int8_t ker_sys_shm_wait( sos_shm_t name );
int8_t ker_sys_shm_stopwait( sos_shm_t name );
sos_pid_t ker_get_current_pid( void );
sos_pid_t ker_get_caller_pid( void );
int8_t ker_sys_routing_register( uint8_t fid );

#ifdef SOS_SIM
#ifdef _MODULE_
#include <hardware_types.h>
extern sos_pid_t ker_get_current_pid( void );
extern uint16_t node_address;
#endif
#endif
/// \endcond 
#endif

/**
 * \defgroup system_api SOS System API
 */

/**
 * \ingroup system_api
 * \defgroup malloc Memory Allocation
 * Functions for memory management
 * @{
 */

/// \cond NOTYPEDEF
typedef void *  (* sys_malloc_ker_func_t)( uint16_t size );
/// \endcond

/**
 * Allocate memory
 *
 * \param size Number of bytes to allocate
 *
 * \return Pointer to memory 
 *
 * \note
 * A call to sys_malloc will either return a valid pointer or, upon failure,
 * cause the system to enter a ``panic'' mode.  The default panic mode
 * suspends all node operations and sends a panic message out via: LEDs,
 * radio, and UART.
 *
 * Remember to either store, free, or SOS_RELEASE any dynamically allocated
 * memory.  Leaky motes sink boats!  Or is that lips...
 *
 */
static inline void *  sys_malloc( uint16_t size )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_malloc_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*1))(size); 
#else
	return ker_sys_malloc( size );
#endif
}

/// \cond NOTYPEDEF
typedef void *  (* sys_realloc_ker_func_t)( void *  ptr, uint16_t newSize );
/// \endcond

/**
 * Reallocate dynamic memory
 * 
 * \param ptr Pointer to the currently held block of memory
 *
 * \param newSize Number of bytes to be allocated
 *
 * \return Pointer to the reallocated memory.
 *
 * This function is a slightly optimized way to extend the size of a buffer.
 * If it can, the function will simply extend the current block of memory so
 * that no data needs to be copied and return the ptr passed in.  If that
 * fails the kernel will attempt to allocate a fresh larger buffer, copy the
 * data over, and return a pointer to this new buffer.  If neither of these
 * two succeed, the function will enter ``panic'' mode.
 *
 * \note Returns a NULL if unable to reallocate but the original pointer is
 * still valid.
 *
 */
static inline void *  sys_realloc( void *  ptr, uint16_t newSize )
{
#ifdef SYS_JUMP_TBL_START                                                               
	return ((sys_realloc_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*2))( ptr, newSize );                                
#else
	return ker_sys_realloc( ptr, newSize );
#endif
} 


/// \cond NOTYPEDEF                                             
typedef void (* sys_free_ker_func_t)( void *  ptr );            
/// \endcond  
/**
 *
 * Free memory
 *
 * \param ptr Pointer to the data that should be freed
 *
 */
static inline void sys_free( void *  ptr )                      
{                                          
#ifdef SYS_JUMP_TBL_START 
	((sys_free_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*3))( ptr );  
#else
	ker_sys_free( ptr );
#endif
}   

/// \cond NOTYPEDEF                                             
typedef void *  (* sys_msg_take_data_ker_func_t)( Message *  msg );
/// \endcond        
/**
 *
 * Claim the data payload of a message
 *
 * \param msg Pointer to message structure carrying data to claim 
 *
 * \return Pointer to memory
 *
 * This function allows you to claim the data in an in coming message.  This
 * is often called from the message handler function of a module.  Module
 * writers can treat this function in a manner very similar to the
 * sys_malloc() function.
 *
 * \n If a module does not call this function, the msg->data field may be
 * released by the kernel or another module after the current function ends.
 * This can make for very difficult to track bugs.
 *
 * \n The lower level behavior of this function is based upon if the
 * ::SOS_MSG_RELEASE flag was set by the call to post(), or one if its
 * variations, that generated the message.
 *
 * \li If ::SOS_MSG_RELEASE was set then this function takes control of the
 * data released into the data payload of the message
 *
 * \li If ::SOS_MSG_RELEASE was NOT set then this function attempts to
 * allocate a new buffer of the same size and create a deep copy of the data
 * payload
 *
 * \note A call to sys_msg_take_data will either return a valid pointer or,
 * upon failure, cause the system to enter a ``panic'' mode.  The default
 * panic mode suspends all node operations and sends a panic message out via:
 * LEDs, radio, and UART.
 *
 */
static inline void *  sys_msg_take_data( Message *  msg )       
{
#ifdef SYS_JUMP_TBL_START                                                               
	return ((sys_msg_take_data_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*4))( msg );                                         
#else
	return ker_sys_msg_take_data( msg );
#endif
}        

/* @} */



/**
 * \ingroup system_api
 * \defgroup timer Timer
 * Functions to work with software timers in the system.
 *
 * @{
 */
/// \cond NOTYPEDEF                                             
typedef int8_t (* sys_timer_start_ker_func_t)( uint8_t tid, int32_t interval, uint8_t type );
/// \endcond  

/**
 * Start a timer
 * 
 * \param tid Timer ID. This ID has to be defined by the caller.
 * 
 * \param interval The timeout interval specified in ticks of duration (1/1024) sec.
 *
 * \param type Timer type - Periodic or One Shot (Defined in sos_timer.h)
 * 
 * \return SOS_OK upon success, else -EINVAL
 *
 * in a failure with error code -EINVAL.
 * 
 */
static inline int8_t sys_timer_start( uint8_t tid, int32_t interval, uint8_t type )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_timer_start_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*5))( tid, interval, type );
#else
	return ker_sys_timer_start( tid, interval, type );
#endif
}


/// \cond NOTYPEDEF                                             
typedef int8_t (* sys_timer_restart_ker_func_t)( uint8_t tid, int32_t interval );
/// \endcond    
/**
 * Restart a timer
 *
 * \param tid Timer ID. This ID has to be defined by the caller.
 * 
 * \param interval The timeout interval specified in ticks of duration (1/1024) sec.
 * 
 *
 * \return SOS_OK upon success, else -EINVAL
 *
 * \warning Re-starting a timer without initializing it will result 
 * in a failure with error code -EINVAL.
 *
 * \note
 * 
 * 
 * \li A running timer is stopped and re-started with the new parameters.
 * 
 * \li If the timer is not running, it is started.
 * 
 */
static inline int8_t sys_timer_restart( uint8_t tid, int32_t interval )
{    
#ifdef SYS_JUMP_TBL_START
	return ((sys_timer_restart_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*6))( tid, interval );
#else
	return ker_sys_timer_restart( tid, interval );
#endif
}


/// \cond NOTYPEDEF                                             
typedef int8_t (* sys_timer_stop_ker_func_t)( uint8_t tid );    
/// \endcond         
/**
 * Stop a running timer
 *
 * \param tid Timer ID. This ID has to be defined by the caller.
 *
 * \return SOS_OK on success, else -EINVAL
 *
 * \warning Stopping a timer that is not running or that is 
 * not initialized  will result in a failure with error code -EINVAL.
 *
 */
static inline int8_t sys_timer_stop( uint8_t tid ) 
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_timer_stop_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*7))( tid );
#else
	return ker_sys_timer_stop( tid );
#endif
}   
/* @} */


/**
 * \ingroup system_api
 * \defgroup message Messaging
 * Functions to send and receive SOS Messages
 * @{
 */

/// \cond NOTYPEDEF                                             
typedef int8_t (* sys_post_ker_func_t)( sos_pid_t dst_mod_id, uint8_t type, uint8_t size, void *  data, uint16_t flag );
/// \endcond         

/** Post a message with payload to a module
 *
 * \param dst_mod_id ID of the destination module
 *
 * \param type Unique message identifier. Kernel message types are defined in
 * message_types.h
 *
 * \param size Size of the payload (in bytes) that is being dispatched as a
 * part of the message
 *
 * \param *data Pointer to the payload buffer that is dispatched in the
 * message.
 *
 * \param flag Control scheduler priority, memory management properties of
 * payload.  Check message_types.h
 *
 * \return SOS_OK on success, -ENOMEM on failure
 *
 * \warning MESSAGE PAYLOAD SHOULD NEVER BE ALLOCATED FROM THE STACK.
 *
 * \note Important information about message flags
 * 
 *
 * \li ::SOS_MSG_RELEASE flag should be set if the source module wishes to
 * transfer ownership of the message payload to the destination module.
 *
 * \li ::SOS_MSG_RELIABLE flag is set if the source module wishes to receive
 * notification regarding the success/failure of message delivery to the
 * destination module.  The notification message has the type
 * ::MSG_PKT_SENDDONE and its payload contains the original message. The flag
 * field of the notification message has the value ::SOS_MSG_SEND_FAIL if the
 * message was not successfully delivered. The flag field is set to 0 upon
 * successful delivery.
 *
 * \li ::SOS_MSG_HIGH_PRIORITY flag is set to insert the message in a high
 * priority queue.  The high priority queue is serviced before the low
 * priority queue.
 *
 */
static inline int8_t sys_post( sos_pid_t dst_mod_id, uint8_t type, uint8_t size, void *  data, uint16_t flag )
{                 
#ifdef SYS_JUMP_TBL_START                                              
	return ((sys_post_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*8))( dst_mod_id, type, size, data, flag );          
#else
	return ker_sys_post( dst_mod_id, type, size, data, flag );
#endif
}             

/// \cond NOTYPEDEF
typedef int8_t (* sys_post_link_ker_func_t)( sos_pid_t dst_mod_id, uint8_t type, uint8_t size, void *  data, uint16_t flag, uint16_t dst_node_addr );
/// \endcond

/**
 * Post message with paylaod over different network link.
 *
 * \warning
 *<STRONG>THIS IS NOT A STANDARD INTERFACE. DO NOT USE THIS IN THE MODULE.<br>
 * INSTEAD USE sys_post_net, sys_post_uart OR sys_post_i2c.</STRONG>
 */
static inline int8_t sys_post_link( sos_pid_t dst_mod_id, uint8_t type, uint8_t size, void *  data, uint16_t flag, uint16_t dst_node_addr )
{
#ifdef SYS_JUMP_TBL_START 
	return ((sys_post_link_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*9))( dst_mod_id, type, size, data, flag, dst_node_addr );
#else
	return ker_sys_post_link( dst_mod_id, type, size, data, flag, dst_node_addr );
#endif
}


/** 
 * Post a message with payload over network
 *
 * \param dst_mod_id ID of the destination module
 *
 * \param type Unique message identifier. Kernel message types are defined in
 * message_types.h
 *
 * \param size Size of the payload (in bytes) that is being dispatched as a
 * part of the message
 *
 * \param *data Pointer to the payload buffer that is dispatched in the
 * message.
 *
 * \param flag Control scheduler priority, memory management properties of
 * payload.  Check message_types.h
 *
 * \param dst_node_addr Destination node address
 * 
 * \return SOS_OK on success, -ENOMEM on failure
 *
 * Other than directing the message the radio, this message is the same as
 * sys_post
 *
 */
#define sys_post_net(dst_mod_id, type, size, data, flag, dst_node_addr)   sys_post_link((dst_mod_id), (type), (size), (data), ((flag) | SOS_MSG_RADIO_IO), (dst_node_addr))


/** 
 * Post a message with payload over UART 
 *
 * \param dst_mod_id ID of the destination module
 *
 * \param type Unique message identifier. Kernel message types are defined in
 * message_types.h
 *
 * \param size Size of the payload (in bytes) that is being dispatched as a
 * part of the message
 *
 * \param *data Pointer to the payload buffer that is dispatched in the
 * message.
 *
 * \param flag Control scheduler priority, memory management properties of
 * payload.  Check message_types.h
 *
 * \param dst_node_addr Destination node address
 *
 * \return SOS_OK on success, -ENOMEM on failure
 *
 * Other than directing the message the UART, this message is the same as
 * sys_post
 *
 */
#define sys_post_uart(dst_mod_id, type, size, data, flag, dst_node_addr)   sys_post_link((dst_mod_id), (type), (size), (data), ((flag) | SOS_MSG_UART_IO), (dst_node_addr))

/** 
 * Post a message with payload over I2C 
 *
 * \param dst_mod_id ID of the destination module
 *
 * \param type Unique message identifier. Kernel message types are defined in
 * message_types.h
 *
 * \param size Size of the payload (in bytes) that is being dispatched as a
 * part of the message
 *
 * \param *data Pointer to the payload buffer that is dispatched in the
 * message.
 *
 * \param flag Control scheduler priority, memory management properties of
 * payload.  Check message_types.h
 *
 * \param dst_node_addr Destination node address
 *
 * \return SOS_OK on success, -ENOMEM on failure
 *
 * Other than directing the message the the I2C, this message is the same as
 * sys_post
 *
 */
#define sys_post_i2c(dst_mod_id, type, size, data, flag, dst_node_addr)   sys_post_link((dst_mod_id), (type), (size), (data), ((flag) | SOS_MSG_I2C_IO), (dst_node_addr))

/// \cond NOTYPEDEF                                             
typedef int8_t (* sys_post_value_ker_func_t)( sos_pid_t dst_mod_id, uint8_t type, uint32_t data, uint16_t flag );
/// \endcond

/**
 *  Post up to 4 bytes by value
 *
 * \param dst_mod_id ID of the destination module
 *
 * \param type Unique message identifier. Kernel message types are defined in
 * message_types.h
 *
 * \param data Data to send directly in the message
 *
 * \param flag Control scheduler priority, memory management properties of
 * payload.  Check message_types.h
 *
 * \return SOS_OK on success, -ENOMEM on failure
 *
 * \note This call is used to dispatch messages which have a very small
 * payloads by value.  Single values can be passed directily (with a type
 * cast).  Multiple small values can be passed, but the end user must pack and
 * unpack this data by hand.
 *
 */
static inline int8_t sys_post_value( sos_pid_t dst_mod_id, uint8_t type, uint32_t data, uint16_t flag )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_post_value_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*10))( dst_mod_id, type, data, flag );
#else
	return ker_sys_post_value( dst_mod_id, type, data, flag );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup nodeInfo Node Info
 * Functions that provide information about a node.
 * @{
 */
/// \cond NOTYPEDEF                                             
typedef uint16_t (* sys_hw_type_ker_func_t)( void );             
/// \endcond    
/**
 * Node hardware type
 * 
 * \return ID of hardware type
 *
 * Returns an ID describing the hardware type of the node.  Common hardware
 * types include:
 * 
 * 
 * \li Mica2 -> 1
 * 
 * \li MicaZ -> 2
 * 
 * \li Tmote -> 6
 * 
 * 
 * The detailed listing of hardware types is available in sos_info.h
 *
 */
static inline uint16_t sys_hw_type( void )                       
{
#ifdef SYS_JUMP_TBL_START                                                               
	return ((sys_hw_type_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*11))( );                                             
#else
	return ker_hw_type();
#endif
}     

/// \cond NOTYPEDEF                                             
typedef uint16_t (* sys_id_ker_func_t)( void );                  
/// \endcond    
/**
 * 
 * Node ID
 *
 * \return ID of the node
 *
 * Returns the node's ID.  This ID, much like an IP address, is the identifier
 * for the node in the network.  A node's ID is set at compile time by
 * specifying:
 * 
 * \verbatim make mica2 install ADDRESS=<node_address> \endverbatim
 *
 * when building the image for the node.  The address is explicitly set in the
 * binary loaded onto the node using $(ROOTDIR)/tools/admin/set-mote-id
 * utility.
 *
 */
static inline uint16_t sys_id( void )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_id_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*12))( );
#else
	return ker_id();
#endif
}

/* @} */

/**
 * \ingroup system_api
 * \defgroup random Random Numbers
 * Functions to generate random numbers.
 * @{
 */
/// \cond NOTYPEDEF
typedef uint16_t (* sys_rand_ker_func_t)( void );
/// \endcond

/**
 *
 * Random number
 *
 * \return Pseudo-random number
 *
 * Very simple random number generator.
 *
 */
static inline uint16_t sys_rand( void )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_rand_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*13))();
#else
	return ker_rand();
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup sysTime System Time
 * Functions to access the node system time
 * @{
 */
/// \cond NOTYPEDEF
typedef uint32_t (* sys_time32_ker_func_t)( void );
/// \endcond
/**
 *
 * Get CPU "time"
 *
 * \return Current CPU clock value
 *
 */
static inline uint32_t sys_time32( void )    
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_time32_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*14))( );
#else
	return ker_systime32();
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup sensing Sensing
 * Function to sample sensor readings
 *
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (* sys_sensor_get_data_ker_func_t)( uint8_t sensor_id );
/// \endcond
/**
 * Get sensor readings
 * \return SOS_OK for success.  The reading will appear as message typed MSG_DATA_READY.
 */
static inline int8_t sys_sensor_get_data( uint8_t sensor_id )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_sensor_get_data_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*15))( sensor_id );
#else
	return ker_sys_sensor_get_data( sensor_id );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup leds LED functions
 *
 * @{
 */

/// \cond NOTYPEDEF
/**
 * Function for setting leds for debugging
 */
typedef void (* sys_led_ker_func_t)( uint8_t op );
/// \endcond

/**
 *
 * LEDs
 *
 * \param op Bitmask used to turn on and off LEDs.
 * \return Zero
 * 
 * Legal values of op on the mica2 node incnlude:
 * 
 * \li LED_RED_ON
 * \li LED_GREEN_ON
 * \li LED_YELLOW_ON
 * \li LED_RED_OFF
 * \li LED_GREEN_OFF
 * \li LED_YELLOW_OFF
 * \li LED_RED_TOGGLE
 * \li LED_GREEN_TOGGLE
 * \li LED_YELLOW_TOGGLE
 *
 *
 */
static inline void sys_led( uint8_t op )
{
#ifdef SYS_JUMP_TBL_START
	((sys_led_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*16))( op );
#else
	ker_led( op );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup modst Module State
 *
 * @{
 */
/// \cond NOTYPEDEF
typedef void* (* sys_get_module_state_func_t)( void );
/// \endcond
/**
 * Get module specific state
 * This is mainly used in dynamic functions
 * \return module state
 */
static inline void* sys_get_state( void )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_get_module_state_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*17))( );
#else
	return ker_sys_get_module_state();
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \ingroup dymfunc
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (* sys_fntable_subscribe_func_t)( sos_pid_t pub_pid, uint8_t fid, uint8_t table_index );
/// \endcond

/**
 * Subscribe a function at runtime
 * \param pub_pid the module ID of the function publisher the module intends to subscribe
 * \param fid the function ID the module intends to subscribe
 * \param table_index the index to the function record, starting zero
 * \return SOS_OK
 *
 * \note table_index is the array index in the funct member of the mod_header_t.  table_index is mainly used for 
 * function prototype checking during linking.   
 */
static inline int8_t sys_fntable_subscribe( sos_pid_t pub_pid, uint8_t fid, uint8_t table_index )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_fntable_subscribe_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*18))(pub_pid, fid, table_index);
#else
	return ker_sys_fntable_subscribe( pub_pid, fid, table_index );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \ingroup malloc
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (* sys_change_own_func_t)( void* ptr );
/// \endcond

/**
 * Change the ownership of the memory.  
 * \param ptr the pointer to the dynamically allocated memory
 * \return SOS_OK
 *
 * \note if ptr is not a valid dynamic memory, sys_change_own enters a ``panic'' mode.
 */
static inline int8_t sys_change_own( void* ptr )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_change_own_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*19))(ptr);
#else
	return ker_sys_change_own( ptr );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup codemem Program Memory
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (* sys_codemem_read_func_t)(codemem_t h, void *buf, uint16_t nbytes, uint16_t offset);
/// \endcond

/**
 * Read from the program memory section
 *
 * \param h        Handle to codemem section
 * \param *buf     Buffer for storing the read
 * \param nbytes   Number of bytes in buf. Cannot be larger than FLASH_PAGE_SIZE
 * \param offset   Offset relative to the start of codemem section
 * \return SOS_OK  If read operation succeeded
 */
static inline int8_t sys_codemem_read(codemem_t h, void *buf, uint16_t nbytes, uint16_t offset)
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_codemem_read_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*21))(h, buf, nbytes, offset);
#else
  return ker_sys_codemem_read(h, buf, nbytes, offset);
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup shm Share Memory
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (* sys_shm_open_func_t )( sos_shm_t name, void *shm );
/// \endcond

/**
 * Create a shared memory with the memory 'name' and the shared memory region shm
 * \param name the name of shared memory in numeric value
 * \param shm  the pointer to the shared memory region
 * \return SOS_OK for success or enter a ``panic'' mode when the name already exists or when there is no memory
 */
static inline int8_t sys_shm_open(  sos_shm_t name, void *shm )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_open_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*22))( name, shm );
#else
  return ker_sys_shm_open( name, shm );
#endif
}

/**
 * Bind an existing name to a new memory region
 * 
 * \param name the name of shared memory in numeric value
 * \param shm  the pointer to the shared memory region
 * \return SOS_OK for success or enter a ``panic'' mode if the name does not exist
 *
 * sys_shm_update signals the update of the shared memory region.  
 * This could be either a new memory region or the update of existing memory
 */
static inline int8_t sys_shm_update(  sos_shm_t name, void *shm )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_open_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*23))( name, shm );
#else
  return ker_sys_shm_update( name, shm );
#endif
}

/// \cond NOTYPEDEF
typedef int8_t (* sys_shm_close_func_t )( sos_shm_t name );
/// \endcond

/**
 * Close the shared memory
 * 
 * \param name the name of shared memory in numeric value
 * \return SOS_OK for success or enter a ``panic'' mode if the name does not exist or if the owner of 
 * the shared memory mismatch
 * \note the memory attached to the name is not freed.
 */
static inline int8_t sys_shm_close( sos_shm_t name )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_close_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*24))( name );
#else
  return ker_sys_shm_close( name );
#endif
}

/// \cond NOTYPEDEF
typedef void* (* sys_shm_get_func_t )( sos_shm_t name );
/// \endcond

/**
 * Find the shared memory (if any) that is bound to the name
 *
 * \param name the name of shared memory in numeric value
 * \return the memory region corresponding to the name, NULL if not exist
 */
static inline void* sys_shm_get( sos_shm_t name )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_get_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*25))( name );
#else
  return ker_sys_shm_get( name );
#endif
}

/**
 * Wait on the event from the shared memory
 *
 * \param name the name of the shared memory in numeric value
 * \return SOS_OK for success or enter a ``panic'' mode if the name is invalid or there is no space to 
 * store more waiter
 *
 * sys_shm_wait waits on the event due to share memory update and close for a 
 * particular memory.  A message from KER_SHM_PID typed MSG_SHM will be generated and 
 * sent to the module when the memory is either updated and closed.  When the module receives the MSG_SHM, 
 * module can use shm_get_name to extract the event type.  SHM_UPDATED indicates the memory was updated.
 * SHM_CLOSED indicates the memory was closed.  
 */
static inline int8_t sys_shm_wait( sos_shm_t name )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_close_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*26))( name );
#else
  return ker_sys_shm_wait( name );
#endif
}

/**
 * Stop waiting on shared memory event 
 * \param name the name of the shared memory in numeric value
 * \return SOS_OK for success or enter a ``panic'' mode if the name is invalid or if the waiter does not exist
 */
static inline int8_t sys_shm_stopwait( sos_shm_t name )
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_shm_close_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*27))( name );
#else
  return ker_sys_shm_stopwait( name );
#endif
}
/* @} */

/**
 * \ingroup system_api
 * \defgroup modinfo Module Info
 * @{
 */
/// \cond NOTYPEDEF
typedef sos_pid_t (*sys_pid_func_t)();
/// \endcond

/**
 * Reture the module ID of the caller.  
 */
static inline sos_pid_t sys_pid()
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_pid_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*28))( );
#else
  return ker_get_current_pid( );
#endif
}

/**
 * Return the caller of the function.
 * \note this function will only return meanful ID when it is in the exported function 
 * implementation
 */
static inline sos_pid_t sys_caller_pid()
{
#ifdef SYS_JUMP_TBL_START
  return ((sys_pid_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*29))( );
#else
  return ker_get_caller_pid( );
#endif
}
/* @} */

typedef int8_t (*sys_routing_register_func_t)( uint8_t fid );

static inline int8_t sys_routing_register( uint8_t fid )
{
#ifdef SYS_JUMP_TBL_START
	return ((sys_routing_register_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*29))( fid );
#else
	return ker_sys_routing_register( fid );
#endif
}


#endif


