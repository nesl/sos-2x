/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

// A memory manager based on free list implementation.
// Used only for fixed sized tokens.
// + Constant allocate and de-allocate time.

#include <sos.h>
#include <string.h>
#include "token_capture.h"
#include "vire_malloc.h"

// Maximum number of tokens that can be handled in the 
// system at any given time.
// TO DO: Modify implementation so that there is no
// static limit on the max number of tokens.
#define MAX_NUM_TOKENS	16

// This token manager is only used if the desired
// execution optimization is enabled by defining the 
// flag USE_VIRE_TOKEN_MEM

#ifdef USE_VIRE_TOKEN_MEM
// A block in the token slab. If it is in use,
// its contents can be accessed through variable
// "token". Otherwise, "next" points to the next
// free element in the token slab. 
typedef struct token_table_t {
	union {
		token_type_t token;
		struct token_table_t *next;
	} content;
} token_table_t;

typedef struct {
	token_table_t *token_block;	//!< A pointer to the token slab (comprises MAX_NUM_TOKENS)
	token_table_t *free_list;	//!< A pointer to the free list.
} server_state_t;

static server_state_t st;

static void init_token_block();
#endif

static int8_t module(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = VIRE_MEM_SERVER_PID,
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
		#ifdef USE_VIRE_TOKEN_MEM
			st.token_block = (token_table_t *)ker_malloc(MAX_NUM_TOKENS*sizeof(token_table_t), VIRE_MEM_SERVER_PID);
			if (st.token_block != NULL) init_token_block();
			st.free_list = st.token_block;
		#endif
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

#ifdef USE_VIRE_TOKEN_MEM

static void init_token_block() {
	uint8_t i;

	for (i = 0; i < MAX_NUM_TOKENS-1; i++) {
		st.token_block[i].content.next = &(st.token_block[i+1]);
	}
	st.token_block[i].content.next = NULL;
}

void *token_malloc(sos_pid_t pid) {
	// No more free blocks. Return NULL.
	if (st.free_list == NULL) return NULL;

	// Store a pointer to first free block.
	token_table_t *blk = st.free_list;

	// Advance the free_list to next free block.
	st.free_list = (st.free_list)->content.next;

	// Return stored block.
	return blk;
}

void token_free(void *ptr) {
	// Perform a sanity check on the pointer first.
	//int32_t diff = (int32_t)((uint8_t*)ptr - (uint8_t*)st.token_block);
	token_table_t *t = (token_table_t *)ptr;

	//if ((diff < 0) || ((diff % sizeof(token_table_t)) != 0)) return;

	// Add the block to the head of the free list.
	t->content.next = st.free_list;
	st.free_list = t;
}

#endif

#ifndef _MODULE_
mod_header_ptr vire_mem_server_get_header() {
	return sos_get_header_address(mod_header);
}
#endif


