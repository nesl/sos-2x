/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
#include <sos_inttypes.h>
#include <monitor.h>
#include <sos_info.h>
#include <sos_sched.h>
#include <hardware_types.h>

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Global data declarations
//----------------------------------------------------------------------------
static monitor_cb *cb_list;

//----------------------------------------------------------------------------
//  Funcation declarations
//----------------------------------------------------------------------------
int8_t monitor_init()
{
  cb_list = NULL;
  return SOS_OK;
}

int8_t ker_register_monitor(sos_pid_t pid, uint8_t type, monitor_cb *cb)
{
	cb->mod_handle = ker_get_module(pid);
	if(cb->mod_handle == NULL) return -ESRCH;
	cb->type = type;
	cb->next = NULL;
	if(cb_list == NULL) {
		/**
		 * Current list is empty
		 */
		cb_list = cb;
	} else {
		/**
		 * List is not empty, traverse the list to the end
		 * and add this new CB
		 */
		monitor_cb *curr = cb_list;
		while(curr->next != NULL){ curr = curr->next; }
		curr->next = cb;
	}
	return SOS_OK;
}

int8_t ker_deregister_monitor(monitor_cb *cb)
{
	monitor_cb *curr = cb_list;
	monitor_cb *prev = cb_list;
	while(curr) {
		if(curr == cb) {
			if(curr == prev) {
				/**
				 * Removing the head
				 */
				cb_list = curr->next;	
			} else {
				/**
				 * Removing non-head item in the list
				 */
				prev->next = curr->next;
			}
			return SOS_OK;
		}
		prev = curr;
		curr = curr->next;
	}
	return -EINVAL;
}

void monitor_deliver_incoming_msg_to_monitor(Message *m)
{
	uint8_t type;
	monitor_cb *curr;
#ifdef MSG_TRACE
#ifdef PC_PLATFORM
	msg_trace(m, false);
#endif
#endif
	if(cb_list == NULL) return;
	/**
	 * in SOS, incoming message can be both local and 
	 * from the network
	 */
	if(m->saddr == node_address) {
		/**
		 * local message
		 */
		type = MON_LOCAL;
	} else {
		/**
		 * from network
		 */
		type = MON_NET_INCOMING;
	}
	curr = cb_list;
	while(curr) {
		/**
		 * We only deliver the message if the destination is 
		 * not the monitor.  It does not make sense to 
		 * deliver the message twice
		 */
		if((curr->type & type) != 0 && 
			curr->mod_handle->pid != m->did) {
			msg_handler_t handler = 
				(msg_handler_t)sos_read_header_ptr(
							curr->mod_handle->header,
							offsetof(mod_header_t,module_handler));
			void *handler_state = curr->mod_handle->handler_state;
			handler(handler_state, m);
		}
		curr = curr->next;
	}
}

void monitor_deliver_outgoing_msg_to_monitor(Message *m)
{
	monitor_cb *curr;
#ifdef MSG_TRACE
#ifdef PC_PLATFORM
	msg_trace(m, true);
#endif
#endif
	if(cb_list == NULL) return;
	curr = cb_list;
	while(curr) {
		/**
		 * We only deliver the message if the source is not
		 * the monitor.  It does not make sense to deliver 
		 * the message sent by the monitor.
		 */
		if((curr->type & MON_NET_OUTGOING) != 0 &&
			curr->mod_handle->pid != m->sid) {
			msg_handler_t handler = 
				(msg_handler_t)sos_read_header_ptr(
							curr->mod_handle->header,
							offsetof(mod_header_t,module_handler));
			void *handler_state = curr->mod_handle->handler_state;
			handler(handler_state, m);
		}
		curr = curr->next;
	}
}

#ifdef PC_PLATFORM
static void print_mod_id(sos_pid_t pid)
{
	if(pid <= KER_MOD_MAX_PID) {
		DEBUG_SHORT("%s", ker_pid_name[pid]);
	} else if(pid >= APP_MOD_MIN_PID) {
		DEBUG_SHORT("%s", mod_pid_name[pid - APP_MOD_MIN_PID]); 
	} else {
		DEBUG_SHORT("unknown %d", pid);
	}
}

static void print_msg_name(uint8_t type)
{
	if(type < MOD_MSG_START) {
		DEBUG_SHORT("%s", ker_msg_name[type]);
	} else {
		DEBUG_SHORT("+%d", type - MOD_MSG_START);
	}
}

void msg_trace(Message *msg, bool out)
{
	if(out) {
		DEBUG_SHORT("<OUT>  ");
	} else {
		if(msg->saddr == node_address) {
			DEBUG_SHORT("<LOCAL>");
		} else {
			DEBUG_SHORT("<IN>   ");
		}
	}
	msg_print(msg);
}
void msg_print(Message *msg)
{
	DEBUG_SHORT(" < %d > ", msg->daddr);
	print_mod_id(msg->did);
	DEBUG_SHORT("   <--- ");
	print_msg_name(msg->type);
	DEBUG_SHORT("( %d ) ----   < %d > ", msg->len, msg->saddr);
	print_mod_id(msg->sid);
	DEBUG_SHORT("\n");
}
#endif

