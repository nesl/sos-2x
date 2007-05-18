/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <sos.h>
#include <codemem.h>
#include "spawn_copy_server.h"

#define HEADER_PTR_INVALID 0xFFFF

typedef struct hdr_copy_elm_t {
	sos_pid_t modID;
	codemem_t cm;
	struct hdr_copy_elm_t *next;
} hdr_copy_elm_t;

typedef struct {
	hdr_copy_elm_t *hqueue;
} server_state_t;


static server_state_t st;

void queue_insert(hdr_copy_elm_t **head, hdr_copy_elm_t *hdr);
hdr_copy_elm_t *queue_search(hdr_copy_elm_t *head, sos_pid_t mid);
void queue_remove(hdr_copy_elm_t **head, hdr_copy_elm_t *elm);

static int8_t module(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = SPAWN_COPY_SERVER_PID,
  .state_size     = 0,
  .num_timers	  = 0,
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .module_handler = module,
};

static int8_t module(void *state, Message *msg) {
	switch (msg->type) {
		case MSG_INIT:
		{
			st.hqueue = NULL;
			break;
		}
		case MSG_FINAL:
		{
			break;
		}
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

mod_header_ptr copy_module_header(sos_code_id_t cid) {
	// Get the pointer to original module header.
	mod_header_ptr src = ker_codemem_get_header_from_code_id(cid);
	uint8_t num_sub_func;
	uint8_t num_prov_func;
	uint8_t *hdr_copy = NULL;
	size_t hdr_size;
	codemem_t cm;
	uint32_t phy_addr;

	// Unable to get original module header. Return HEADER_PTR_INVALID.
	if (src == HEADER_PTR_INVALID) return src;

	// Get number of subscribed and published functions to
	// calculate the size of function control block table.
	num_sub_func = sos_read_header_byte(src,
					offsetof(mod_header_t, num_sub_func));
	num_prov_func = sos_read_header_byte(src,
					offsetof(mod_header_t, num_prov_func));
	
	hdr_size = sizeof(mod_header_t) + ((num_sub_func+num_prov_func) * sizeof(func_cb_t));
	hdr_copy = ker_malloc(hdr_size, SPAWN_COPY_SERVER_PID);
	if (hdr_copy == NULL) return HEADER_PTR_INVALID;
	
	// Copy the original header from flash into memory.
	phy_addr = sos_get_physical_addr(src, 0);
	//phy_addr = ((uint32_t)src) << 1;
	ker_codemem_direct_read(phy_addr, SPAWN_COPY_SERVER_PID, hdr_copy, hdr_size, 0);

	// Allocate a new flash page to store the copy of the module header.
	cm = ker_codemem_alloc(hdr_size, CODEMEM_TYPE_EXECUTABLE);
	if (cm == CODEMEM_INVALID) {
		ker_free(hdr_copy);
		return HEADER_PTR_INVALID;
	}

	// Write the header to the new page.
	ker_codemem_write(cm, SPAWN_COPY_SERVER_PID, hdr_copy, hdr_size, 0);
	ker_codemem_flush(cm, SPAWN_COPY_SERVER_PID);
	ker_free(hdr_copy);

	// Record the information about this duplicate header
	hdr_copy_elm_t *hdr = ker_malloc(sizeof(hdr_copy_elm_t), SPAWN_COPY_SERVER_PID);
	if (hdr == NULL) {
		// Destroy duplicate module header.
		// Return error.
		ker_codemem_free(cm);
		return HEADER_PTR_INVALID;
	}
	hdr->modID = 0;
	hdr->cm = cm;
	hdr->next = NULL;
	queue_insert(&st.hqueue, hdr);

	// Return the pointer to the duplicate header.
	phy_addr = ker_codemem_get_start_address(cm);
	DEBUG("Physical address of codemem start: %d 0x%x.\n", cm, phy_addr);

	return sos_get_header_ptr(phy_addr);
}

int8_t update_last_module_added(sos_pid_t pid) {
	hdr_copy_elm_t *itr = st.hqueue;

	if ((itr == NULL) || (pid == NULL_PID)) return -EINVAL;

	while (itr->next != NULL) {
		itr = itr->next;
	}
	itr->modID = pid; 
	return SOS_OK;
}

int8_t delete_module_header(sos_pid_t pid) {
	hdr_copy_elm_t *hdr = queue_search(st.hqueue, pid);

	if (hdr == NULL) return SOS_OK;

	ker_codemem_free(hdr->cm);
	queue_remove(&st.hqueue, hdr);

	return SOS_OK;
}

//--------------------------------------------------------
// Internal functions
//--------------------------------------------------------
void queue_insert(hdr_copy_elm_t **head, hdr_copy_elm_t *hdr) {
	if (*head == NULL) {
		*head = hdr;
	} else {
		hdr_copy_elm_t *itr = *head;
		while (itr->next != NULL)
			itr = itr->next;
		itr->next = hdr;
	}
}

hdr_copy_elm_t *queue_search(hdr_copy_elm_t *head, sos_pid_t mid) {
	hdr_copy_elm_t *itr = head;

	while (itr != NULL) {
		if (itr->modID == mid) return itr;
		itr = itr->next;
	}

	return NULL;
}

void queue_remove(hdr_copy_elm_t **head, hdr_copy_elm_t *elm) {
	hdr_copy_elm_t *itr = *head;

	if ((itr == NULL) || (elm == NULL)) return;

	if (elm == *head) {
		*head = elm->next;
	} else {
		while (itr->next != elm) {
			itr = itr->next;
		}
		itr->next = elm->next;
	}
	elm->next = NULL;
	ker_free(elm);
}

//------------------------------------------------------

#ifndef _MODULE_
mod_header_ptr spawn_copy_server_get_header() {
	return sos_get_header_address(mod_header);
}
#endif


