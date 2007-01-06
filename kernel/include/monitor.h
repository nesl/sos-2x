/* -*- Mode: C; tab-width:4 -*- */

/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _MONITOR_H

#define _MONITOR_H

#include <pid.h>

#include <sos_types.h>

#include <sos_module_types.h>

#include <message_types.h>

/**

 * @brief message interest type

 */

enum {

	MON_NET_INCOMING = 0x01,

	MON_NET_OUTGOING = 0x02,

	MON_LOCAL        = 0x04,

};

/**

 * @brief monitor control block

 * The block is allocated by module

 * During registeration, this block is handed to SOS

 */

typedef struct monitor_str {

	uint8_t type;              //!< monitor type

	sos_module_t  *mod_handle;  //!< handle to the module	

	struct monitor_str *next;  //!< next pointer

} monitor_cb;



#ifndef _MODULE_

#ifndef QUALNET_PLATFORM



//! Initialize the monitor

int8_t monitor_init(void);



/**

 * @brief register a monitor

 * @param pid registering module id

 * @param type mointoring type, bit vector

 * @param cb  monitor control block

 */

extern int8_t ker_register_monitor(sos_pid_t pid, uint8_t type, monitor_cb *cb);



/**

 * @brief deregister a monitor

 * @param cb  monitor control block

 */

extern int8_t ker_deregister_monitor(monitor_cb *cb);



/**

 * @brief deliver incoming message to monitor manager

 * This is an internal function used to deliver messages to the manager.

 * Manager is then dispatch the message to all interested modules.

 */

extern void monitor_deliver_incoming_msg_to_monitor(Message *m);



/**

 * @brief deliver outgoing message to monitor manager

 * This function is used to deliver the messages that are going out of

 * this node.

 */

extern void monitor_deliver_outgoing_msg_to_monitor(Message *m);



#ifdef PC_PLATFORM

extern void msg_trace(Message *msg, bool out);

extern void msg_print(Message *msg);

#endif

#endif //#QUALNET_PLATFORM

#endif  //_MODULE_



#endif

