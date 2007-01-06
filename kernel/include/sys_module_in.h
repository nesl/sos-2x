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

/**
 *
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
void* ker_sys_malloc(uint16_t size);

/**
 * 
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
 * fails the kernel w ill attempt to allocate a fresh larger buffer, copy the
 * data over, and return a pointer to this new buffer.  If neither of these
 * two succeed, the function returns NULL, but you still have access to ptr.
 *
 * \note Returns a NULL if unable to reallocate but the original pointer is
 * still valid.
 *
 */
void* ker_sys_realloc(void *ptr, uint16_t newSize);


/**
 *
 * Free memory
 *
 * \param ptr Pointer to the data that should be freed
 *
 */
void ker_sys_free(void *ptr);


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
void* ker_sys_msg_take_data(Message *msg);



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
 * \warning Starting a timer without initializing it will result 
 * in a failure with error code -EINVAL.
 * 
 */
int8_t ker_sys_timer_start(uint8_t tid, int32_t interval, uint8_t type);


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
int8_t ker_sys_timer_restart(uint8_t tid, int32_t interval) ;


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
int8_t ker_sys_timer_stop(uint8_t tid) ;


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
int8_t ker_sys_post(sos_pid_t dst_mod_id, uint8_t type,
		uint8_t size, void *data, uint16_t flag);

/**
 * Post message with paylaod over different network link.
 *
 * \warning This is the implementation of sys_post_net, sys_post_uart, and 
 * sys_post_i2c.  Do not use this directly as the implementation might change
 */
int8_t ker_sys_post_link(sos_pid_t dst_mod_id, uint8_t type,
		uint8_t size, void *data, uint16_t flag, uint16_t dst_node_addr);

/** Post a message with payload over network
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


/** Post a message with payload over UART 
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

/** Post a message with payload over I2C 
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


/** Post up to 4 bytes by value
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
int8_t ker_sys_post_value(sos_pid_t dst_mod_id,  
		uint8_t type, uint32_t data, uint16_t flag);

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
uint16_t sys_hw_type(void);

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
uint16_t sys_id(void);


/**
 *
 * Random number
 *
 * \return Pseudo-random number
 *
 * Very simple random number generator.
 *
 */
uint16_t sys_rand(void);

/**
 *
 * Get CPU "time"
 *
 * \return Current CPU clock value
 *
 */
uint32_t sys_time32(void);

#endif


