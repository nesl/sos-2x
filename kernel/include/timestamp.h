
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_
#include <sos_types.h>
#include <message_types.h>

//! use 27 bytes on mica2
#define TIMESTAMP_DEFAULT_SIZE   4

#ifndef _MODULE_
/**
 * @brief register timestamping service
 * @param pid module ID
 * @param n   time stamp buffer size
 * @return SOS_OK for success, errno for error
 *
 * Module uses this to register itself with timestamping service
 * When there is incoming and outgoing messages, the timestamp will be 
 * recorded along with msg->data field.
 * Module uses ker_timestamp_query to get the timestamp
 * One can always use TIMESTAMP_DEFAULT_SIZE when not sure
 */
int8_t ker_timestamp_register(sos_pid_t pid, uint8_t n);

/**
 * @brief deregister timestamp service
 */
int8_t ker_timestamp_deregister(sos_pid_t pid);

/**
 * @brief query the time stamp of a message
 * @param pid module ID
 * @param data msg->data field
 * @return timestamp for success, 0 for fail
 *
 * Each timestamp is associated with msg->data for both 
 * incoming and outgoing messages
 * 
 */
uint32_t ker_timestamp_query(sos_pid_t pid, void *data);


/**
 * @brief telling timestamp service that a message is in
 * Used by radio driver
 */
void timestamp_incoming(Message *msg_in, uint32_t t);

/**
 * @brief telling timestamp service that a message is sent
 * Used by radio driver
 */
void timestamp_outgoing(Message *msg_out, uint32_t t);
#endif /* ifndef _MODULE_ */

#endif
