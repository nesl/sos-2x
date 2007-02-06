/*									tab:4
 *
 *
 * "Copyright (c) 2000-2002 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without written
 * agreement is hereby granted, provided that the above copyright
 * notice, the following two paragraphs and the author appear in all
 * copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
 * CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 */
/*
 * Authors:   Rahul Balani
 * History:   November 2005		Inception
 */

/**
 * @author Rahul Balani
 */

/*
#include <malloc.h>
#include <fntable.h>
#include <malloc_conf.h>
#include <sos_sched.h>
#include <codemem.h>
*/
#include <module.h>
#include <loader/loader.h>

#ifdef LINK_KERNEL
//#define _MODULE_
#endif

#include <VM/DVMResourceManager.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMBasiclib.h>

//void set_start_cond();

typedef struct {
	func_cb_ptr init_event;
	sos_pid_t pid;
	DvmState *scripts[DVM_CAPSULE_NUM];
	uint8_t script_block_owners[DVM_NUM_SCRIPT_BLOCKS];
	DvmState *script_block_ptr;
} app_state;

static int8_t resmanager_handler(void *state, Message *msg) ;

#ifndef _MODULE_
static sos_module_t resmngr_module;
static app_state resmngr_state;

static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id         = M_RESOURCE_MANAGER,
	.code_id        = ehtons(M_RESOURCE_MANAGER),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = resmanager_handler,
};

int8_t resmanager_init()
{
	return sched_register_kernel_module(&resmngr_module, sos_get_header_address(mod_header), NULL);
}

#else
static DvmState *mem_allocate(func_cb_ptr p, uint8_t capsuleNum);
static int8_t mem_free(func_cb_ptr p, uint8_t capsuleNum);
static const mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id         = M_RESOURCE_MANAGER,
	.code_id        = ehtons(M_RESOURCE_MANAGER),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 1,
	.num_prov_func  = 2,
	.module_handler = resmanager_handler,
	.funct          = {
		USE_FUNC_initEventHandler(M_HANDLER_STORE, INITIALIZEHANDLER),
		PRVD_FUNC_mem_allocate(M_RESOURCE_MANAGER, ALLOCATEMEM),
		PRVD_FUNC_mem_free(M_RESOURCE_MANAGER, FREEMEM),
	},
};  

#ifdef LINK_KERNEL
int8_t resmanager_init()
{
	    return ker_register_module(sos_get_header_address(mod_header));
}
#endif

#endif

static int8_t resmanager_handler(void *state, Message *msg){
#ifndef _MODULE_
	app_state *s = &resmngr_state;
#else
	app_state *s = (app_state *) state;
#endif
	switch (msg->type) {
		case MSG_INIT: 
		{
			uint8_t i;
			s->pid = msg->did;
			s->script_block_ptr = (DvmState *)ker_malloc(DVM_DEFAULT_MEM_ALLOC_SIZE, s->pid);
			DEBUG("script block got this addr %08x \n", (uint32_t)s->script_block_ptr);
			if (s->script_block_ptr == NULL)
				DEBUG("No space for script blocks\n");
			for (i = 0; i < DVM_CAPSULE_NUM; i++) {
				s->scripts[i] = NULL;
			}
			for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++)
				s->script_block_owners[i] = NULL_CAPSULE;

			DEBUG("Resource Manager: Initialized\n");
			break;
		}
		case MSG_LOADER_DATA_AVAILABLE:
		{
			MsgParam* params = ( MsgParam * )( msg->data );
			uint8_t id = params->byte;
			codemem_t cm = params->word;
			DvmState *ds;

			DEBUG("RESOURCE_MANAGER: Data received. ID - %d.\n", id);
#ifndef _MODULE_
			ds = mem_allocate(id);
#else
			ds = mem_allocate(0, id);
#endif
			if( ds == NULL ) { 
				// XXX may want to set a timer for retry
				return -ENOMEM;
			}

			ds->context.which = id;

			ker_codemem_read(cm, M_RESOURCE_MANAGER,
					&(ds->context.moduleID), 
					sizeof(ds->context.moduleID), offsetof(DvmScript, moduleID));

			DEBUG("RESOURCE_MANAGER: Event - module = %d, offset = %ld \n", ds->context.moduleID, offsetof(DvmScript, moduleID));

			ker_codemem_read(cm, M_RESOURCE_MANAGER,
					&(ds->context.type), sizeof(ds->context.type),
					offsetof(DvmScript, eventType));

			DEBUG("RESOURCE_MANAGER: Event - event type = %d, offset = %ld \n", ds->context.type, offsetof(DvmScript, eventType));


			// ADDED for evaluation
			//if (ds->context.type == 32) 
			//	set_start_cond();




			ker_codemem_read(cm, M_RESOURCE_MANAGER,
					&(ds->context.dataSize), sizeof(ds->context.dataSize),
					offsetof(DvmScript, length));
			ds->context.dataSize = ehtons(ds->context.dataSize);

			DEBUG("RESOURCE_MANAGER: Length of script = %d, offset = %ld \n", ds->context.dataSize, offsetof(DvmScript, length));

			ker_codemem_read(cm, M_RESOURCE_MANAGER,
					&(ds->context.libraryMask), sizeof(ds->context.libraryMask),
					offsetof(DvmScript, libraryMask));

			DEBUG("RESOURCE_MANAGER: Library Mask = 0x%x, offset = %ld \n", ds->context.libraryMask, offsetof(DvmScript, libraryMask));

			ds->cm = cm;

			initEventHandlerDL(s->init_event, ds, id);
			break;

		}
		case MSG_FINAL:
		{
			uint8_t i;
			for (i = 0; i < DVM_CAPSULE_NUM; i++) {
#ifndef _MODULE_
				mem_free(i);
#else
				mem_free(0, i);
#endif
			}
			ker_free(s->script_block_ptr);
			break;
		}
		default:
		break;
	}

	return SOS_OK;
}

// Allocate continuous space to context, stack and local variables.
#ifndef _MODULE_
DvmState *mem_allocate(uint8_t capsuleNum)
{
	app_state *s = &resmngr_state;
#else
static DvmState *mem_allocate(func_cb_ptr p, uint8_t capsuleNum)
{
	app_state *s = (app_state*)ker_get_module_state(M_RESOURCE_MANAGER);
#endif
	uint8_t i;
	int8_t script_index = -1;

	if (capsuleNum >= DVM_CAPSULE_NUM) return NULL;

	if (s->scripts[capsuleNum] == NULL) {
		//Try to allocate space
		for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++) {
			if (s->script_block_owners[i] == NULL_CAPSULE) {
				script_index = i;
				s->script_block_owners[script_index] = capsuleNum;
				s->scripts[capsuleNum] = s->script_block_ptr + script_index;
				break;
			}
		}
		if (script_index < 0) {
			s->scripts[capsuleNum] = (DvmState *)ker_malloc(DVM_STATE_SIZE, s->pid);
			if (s->scripts[capsuleNum] == NULL) return NULL;
		}
		// If control reaches here, this means space has been successfully
		// allocated for context etc.
	}

	memset(s->scripts[capsuleNum], 0, DVM_STATE_SIZE);
	return s->scripts[capsuleNum];
}

#ifndef _MODULE_
int8_t mem_free(uint8_t capsuleNum)
{
	app_state *s = &resmngr_state;
#else
static int8_t mem_free(func_cb_ptr p, uint8_t capsuleNum)
{
	app_state *s = (app_state*)ker_get_module_state(M_RESOURCE_MANAGER);
#endif
	uint8_t i;

	//Free script space
	for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++) {
		if (s->script_block_owners[i] == capsuleNum) {
			s->script_block_owners[i] = NULL_CAPSULE;
			break;
		}
	}
	if (i >= DVM_NUM_SCRIPT_BLOCKS) 
		ker_free(s->scripts[capsuleNum]); 
	s->scripts[capsuleNum] = NULL;

	return SOS_OK;
}

