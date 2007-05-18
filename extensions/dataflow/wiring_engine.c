
#include <sos.h>
#include <codemem.h>
#include <loader.h>
#include <string.h>
#include "spawn_copy_server.h"
#include "wiring_config.h"
#include "wiring_config_private.h"
// Debugging
#define LED_DEBUG
#include <led_dbg.h>

// Run-time engine state. The current implementation allows
// multiple application graphs to be installed on the system at
// run-time, but with the restriction that they will share the
// same routing table.
// Allocating one page for routing table restricts the total
// number of wires to be approx 40 wires in the worst case (which
// is 40 output ports, each connected to only one wire). In
// practice, this case hasn't been encountered till now.
// Allocating one page for elements table restricts the max
// number of elements in the graphs to be approx 60.
typedef struct {
	codemem_t routing_table;	//!< A page to maintain routing information
	codemem_t elements_table;	//!< A page to maintain the list of graph elements and their PID's
	codemem_t saved_params_table;	//!< Table to save all the parameters across graph reset
	codemem_t saved_wiring_table;	//!< Table to save the wiring configuration sent by PC
									//!< for use in using diff updates to wiring.
	uint16_t num_elements;		//!< Total number of elements in the graph application.
	uint8_t busy_bit_mask[MAX_NUM_ELEMENTS][(MAX_INPUT_PORTS_PER_ELEMENT+7)/8];	
								//!< Tracking BUSY vs READY states 
								// of input ports of modules
	mesg_queue_t *mqueue;		//!< Pointer to the head of the token queue
	queue_header_t *module_table;	//!< Pointer to elements table when it is loaded in RAM
	routing_table_ram_t *routing_table_ram;	//!< Pointer to the routing table in RAM
	func_cb_t f;		//Debug:
} mcast_state_t;

static mcast_state_t st;

typedef struct {
	func_cb_ptr update_param;
} mcast_dyn_fn_state_t;

// Published functions
//static int8_t dispatch(func_cb_ptr p, func_cb_ptr caller_cb, token_type_t *t);
static int8_t signal_error(func_cb_ptr p, int8_t error);

// Internal functions
static void queue_insert(queue_header_t **head, queue_header_t *elm);
static void queue_remove(queue_header_t **head, queue_header_t *elm, uint16_t size);
static void queue_free(queue_header_t **head, uint16_t size);

static void token_queue_remove(mesg_queue_t **head, uint8_t epid, queue_header_t *elm);
static mesg_queue_t *mesg_queue_insert(mesg_queue_t **head, uint8_t epid);
static void token_queue_free(token_queue_t **head);
static void purge_tokens(queue_header_t *module_table);
static bool tokens_posted_for_element(sos_pid_t epid);
static void mesg_queue_free(mesg_queue_t **head);
static mesg_queue_t *search_mesg_queue(mesg_queue_t *head, sos_pid_t epid);

static inline void reset_busy_mask();
static inline void reset_mesg_queue();

static void reset_engine();
static uint8_t discovered(graph_element_t *head, common_table_header_t *row, sos_pid_t *modID);
static int8_t install_new_configuration(codemem_t cm, uint8_t update_format, 
													size_t *config_file_offset);
static void load_elements_table(queue_header_t **module_table, uint8_t cmd);
static int8_t hot_swap_configuration(codemem_t cm, uint8_t update_format, size_t *config_file_offset);
static int8_t interpret_wiring_config(codemem_t cm, size_t *file_offset, 
										uint16_t length, queue_header_t **module_table);
static int8_t update_parameter_table(uint8_t update_format, queue_header_t *module_table, 
												codemem_t cm, size_t *config_file_offset);
static int8_t apply_parameter_update(mcast_dyn_fn_state_t *s);

static sos_module_t *process_module(graph_element_t **init_module_table, wiring_table_row_t *record, 
																sos_pid_t *elementID);
static void patch_all_published_functions(mod_header_ptr header, sos_pid_t pid, 
											uint8_t start, uint8_t end);
static void invalidate_output_ports(sos_pid_t pid);

static bool is_element_ready(uint8_t pid_port);
static void set_element_status(sos_pid_t epid, uint8_t portID, uint8_t status);
static bool is_graph_busy();
static void	reset_element_busy_mask(queue_header_t *module_table);
static void post_tokens_for_all_ports(sos_pid_t epid);
static void deregister_modules(queue_header_t *mod_table, uint8_t cmd);
static void remove_unmarked_elements(queue_header_t **module_table);
static bool check_updated_params_for_element(codemem_t cm, size_t file_offset, param_table_row_t row);

//static void read_routing_table_in_flash(codemem_t routing_table, uint16_t gid, routing_table_ram_t *ptr);

#ifdef PUT_ROUTING_TABLE_IN_RAM
static inline void reset_routing_table_in_ram();
static int8_t load_routing_table(routing_table_ram_t **tab, codemem_t cm, uint16_t num_entries);
//static void read_routing_table_in_ram(codemem_t cm, routing_table_ram_t *tab, uint16_t gid, routing_table_ram_t *ptr);
//#define read_routing_table(cm, tab, gid, ptr) (read_routing_table_in_ram(cm, tab, gid, ptr))

#else

#define load_routing_table(tab, cm, num) (0)
//#define read_routing_table(cm, tab, gid, ptr) (read_routing_table_in_flash(cm, gid, ptr))
#define reset_routing_table_in_ram()

#endif

// External function defined in codemem.c
extern func_cb_ptr fntable_real_subscribe(mod_header_ptr sub_h,
        sos_pid_t pub_pid, uint8_t fid, uint8_t table_index);

static int8_t module(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id			= MULTICAST_SERV_PID,
	.state_size		= sizeof(mcast_dyn_fn_state_t),
	.num_sub_func	= 1,
	.num_prov_func	= 1,
	.module_handler	= module,
	.funct			= {
		{error_8, "cwS2", RUNTIME_PID, UPDATE_PARAM_FID},
		//{dispatch, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
		{signal_error, "ccv1", MULTICAST_SERV_PID, SIGNAL_ERR_FID},
	},
};

static int8_t module(void *state, Message *msg) {
	mcast_dyn_fn_state_t *s = (mcast_dyn_fn_state_t *)state;

	switch (msg->type) {
		case MSG_INIT: {
			st.routing_table = CODEMEM_INVALID; 
			st.elements_table = CODEMEM_INVALID; 
			st.saved_wiring_table = CODEMEM_INVALID; 
			st.saved_params_table = CODEMEM_INVALID; 
			st.num_elements = 0;
			st.mqueue = NULL;
			st.module_table = NULL;
			st.routing_table_ram = NULL;
			reset_busy_mask();
			LED_DBG(LED_RED_OFF);
			LED_DBG(LED_YELLOW_OFF);
			LED_DBG(LED_GREEN_OFF);
			LED_DBG(LED_YELLOW_ON);
			break;
		} 
		case MSG_FINAL: { 
			reset_engine();
			break;
		}
		case MSG_CONTINUE_TASK: {
			msg_continue_t *m = (msg_continue_t *)ker_msg_take_data(MULTICAST_SERV_PID, msg);
			int8_t status;

			// Element is ready as we ensured this while posting the task.
			// Mark the token as HANDLED. Used to check whether any unhandled token
			// is still posted for the same destination element.
			m->mptr->status = TOKEN_HANDLED;

			// Release the token as it won't be used by the engine any more.
			release_token(m->mptr->t);

			// TODO: Mark the whole element as BUSY to prevent any race 
			// condition due to feedback.

			// Access the input port.
			status = SOS_CALL(m->mptr->cb, input_func_t, m->mptr->t);
			if (status == -EBUSY) {
				// Element has accepted the current input token
				// and will be performing a long (split-phase) operation on it.
				// Set its bit mask to indicate ELEMENT_BUSY status for future
				// tokens.
				set_element_status(m->epid, m->mptr->portID, ELEMENT_BUSY);
			} else if (status < 0) {
				// Some other error occured. Need to reset the graph.
				// Post a message (RESET task, HIGHEST priority) to itself.
				//graph_reset();
				break;
			}
			// Destroy the token if it is no longer required (i.e. locked = 0).
			destroy_token(m->mptr->t);

			// Remove the token from the queue.
			token_queue_remove(&st.mqueue, m->epid, (queue_header_t *)(m->mptr));

			// Free the task memory
			vire_free(m, sizeof(msg_continue_t));

			break;
		}
		case MSG_TIMER_TIMEOUT: { 
			break;
		}
		case MSG_LOADER_DATA_AVAILABLE: {
			MsgParam* p = (MsgParam *)(msg->data);
			codemem_t cm = p->word;
			size_t config_file_offset = LOADER_METADATA_SKIP_OFFSET;
			int8_t ret = 0;

			DEBUG("Wiring Engine gets data from loader\n");
			
			// Allocate fresh pages in flash for routing table and elements list,
			// if not already done so.
			// Their size can only be known after interpreting configuration file,
			// so they are pre-allocated a fixed amount of space to reduce memory
			// consumption while installing the graph.
			// Proceed with graph initialization if allocation is successful.
			// Free the pages when the graph is stopped or initialization fails.
			// Also, we could send back an error report so that appropriate action
			// can be taken.

			if (st.routing_table == CODEMEM_INVALID) {
				st.routing_table = ker_codemem_alloc(FLASHMEM_PAGE_SIZE, CODEMEM_TYPE_EXECUTABLE);
				if (st.routing_table == CODEMEM_INVALID) {
					ker_codemem_free(cm);
					return -ENOMEM;
				}
			}

			if (st.elements_table == CODEMEM_INVALID) {
				uint8_t buf = 0;
				st.elements_table = ker_codemem_alloc(FLASHMEM_PAGE_SIZE, CODEMEM_TYPE_EXECUTABLE);
				if (st.elements_table == CODEMEM_INVALID) {
					ker_codemem_free(st.routing_table);
					ker_codemem_free(cm);
					return -ENOMEM;
				}
				// The first byte of elements table gives the total number of elements in the table
				ker_codemem_write(st.elements_table, MULTICAST_SERV_PID, &buf, sizeof(buf), 0);
			}

			DEBUG("Allocated pages in flash for new graph.\n");
			
			// Check bit mask sent in p->byte to determine type of update action
			// required.
			// First, setup graph wiring if required.
			if (p->byte & WIRING_SECTION) {
				// Also update parameter list and post corresponding task.
				// This is handled by these functions also.
				if (p->byte & HOT_SWAP_BIT) {
					ret = hot_swap_configuration(cm, p->byte, &config_file_offset);
				} else {
					ret = install_new_configuration(cm, p->byte, &config_file_offset);
				}

				if (ret < 0) {
					DEBUG("Could not (re)initialize graph. Quitting.\n");
					ker_codemem_free(cm);
					reset_engine();
					return -EINVAL;
				}
			} else if (p->byte & PARAM_SECTION) {
				// Update file only contains new parameters.
				DEBUG("Config file contains parameter section.\n");

				
				// Load elements table into RAM
				load_elements_table(&st.module_table, LOAD);
				if (st.module_table == NULL) {
					// Could not load elements table into RAM. Fail.
					ker_codemem_free(cm);
					return -ENOMEM;
				}

				// Update the saved parameter table and apply the updates.
				if (update_parameter_table(p->byte, st.module_table, cm, &config_file_offset) 
																					== SOS_OK) {
					apply_parameter_update(s);
				}
				queue_free(&st.module_table, sizeof(graph_element_t));

			}

			ker_codemem_free(cm);

			return ret;
		}
		case MSG_UPDATE_PARAMETERS:
		{
			apply_parameter_update(s);
			queue_free(&st.module_table, sizeof(graph_element_t));

			break;
		}
		default: 
			return -EINVAL;
	}

	return SOS_OK;
}

static inline void reset_busy_mask() {
	memset(st.busy_bit_mask, 0xFF, ((MAX_INPUT_PORTS_PER_ELEMENT+7)/8)*MAX_NUM_ELEMENTS);
}

static inline void reset_mesg_queue() {
	mesg_queue_free(&st.mqueue);
	st.mqueue = NULL;
}


static void reset_engine() {
	ker_codemem_free(st.routing_table);
	ker_codemem_free(st.elements_table);
	ker_codemem_free(st.saved_params_table);
	ker_codemem_free(st.saved_wiring_table);
	st.routing_table = CODEMEM_INVALID;
	st.elements_table = CODEMEM_INVALID;
	st.saved_params_table = CODEMEM_INVALID;
	st.saved_wiring_table = CODEMEM_INVALID;
	st.num_elements = 0;
	queue_free(&st.module_table, sizeof(graph_element_t));
	st.module_table = NULL;
	reset_routing_table_in_ram();
	reset_mesg_queue();
	reset_busy_mask();
}

static int8_t install_new_configuration(codemem_t cm, uint8_t update_format, 
													size_t *config_file_offset)
{
	section_header_t record;
	int8_t ret = SOS_OK;

	// Reset busy mask and token queues in engine.
	reset_busy_mask();
	reset_mesg_queue();
	reset_routing_table_in_ram();

	// Read the table from flash and deregister all modules.
	// Note: the table has NOT been loaded onto flash because
	// of option DEREGISTER.
	load_elements_table(&st.module_table, DEREGISTER);

	// Read first section header from wiring config file
	ker_codemem_read(cm, MULTICAST_SERV_PID, &record, sizeof(section_header_t), *config_file_offset);
	(*config_file_offset) += sizeof(section_header_t);

	// Signal error if its not wiring header.
	if (record.type != BEGIN_WIRING) {
		// Corrupted config packet.
		DEBUG("Expecting WIRING section header, but found something else.\n");
		return -EINVAL;
	}

	// Setup wiring according to update format. 
	if (update_format & UPDATE_DIFF) {
		// Make a new wiring config from from saved wiring table and cm
		// Call interpret_wiring_config() with new wiring table
		ret = SOS_OK;
	} else {
		// It is a COMPLETE wiring config.
		ret = interpret_wiring_config(cm, config_file_offset, record.length, &st.module_table);
	}

	if (ret < 0) {
		deregister_modules(st.module_table, ALL);
		queue_free(&st.module_table, sizeof(graph_element_t));
		return ret;
	}

	// The graph was initialized properly.
	// Check if the parameters need to be updated.
	if (update_format & PARAM_SECTION) {
		// Remove unmarked elements from elements table
		remove_unmarked_elements(&st.module_table);

		// Update the parameters list, and post a UPDATE_PARAMETERS task
		ret = update_parameter_table(update_format, st.module_table, cm, config_file_offset);
		if (ret < 0) {
			// Update parameter table failed. Do not apply any updates.
			// Free the module table.
			// The application will run with default parameters.
			// Signal the base station about this error.
			queue_free(&st.module_table, sizeof(graph_element_t));
			return SOS_OK;
		}
		post_short(MULTICAST_SERV_PID, MULTICAST_SERV_PID, MSG_UPDATE_PARAMETERS, 0, 0, 0);
	} else if (update_format & MERGE_PARAMETERS) {
		post_short(MULTICAST_SERV_PID, MULTICAST_SERV_PID, MSG_UPDATE_PARAMETERS, 0, 0, 0);
	} else {
		queue_free(&st.module_table, sizeof(graph_element_t));
	}

	return ret;

}

static int8_t hot_swap_configuration(codemem_t cm, uint8_t update_format, size_t *config_file_offset)
{
	section_header_t record;
	int8_t ret = SOS_OK;

	// Check if any element in graph is BUSY.
	if (is_graph_busy()) {
		// It is not possible to hot swap the new configuration
		// so, fall back to installing a complete new graph
		return install_new_configuration(cm, update_format, config_file_offset);
	}

	// Destory the routing table in RAM
	// as it will be set up again
	reset_routing_table_in_ram();

	// Load the elements into memory
	load_elements_table(&st.module_table, LOAD);
	if (st.module_table == NULL) {
		return -ENOMEM;
	}

	// Read first section header from wiring config file
	ker_codemem_read(cm, MULTICAST_SERV_PID, &record, sizeof(section_header_t), *config_file_offset);
	(*config_file_offset) += sizeof(section_header_t);

	// Signal error if its not wiring header.
	if (record.type != BEGIN_WIRING) {
		// Corrupted config packet.
		DEBUG("Expecting WIRING section header, but found something else.\n");
		return -EINVAL;
	}

	// Setup wiring according to update format. 
	if (update_format & UPDATE_DIFF) {
		// Make a new wiring config from from saved wiring table and cm
		// Call interpret_wiring_config() with new wiring table
		ret = SOS_OK;
	} else {
		// It is a COMPLETE wiring config.
		ret = interpret_wiring_config(cm, config_file_offset, record.length, &st.module_table);
	}

	if (ret < 0) {
		deregister_modules(st.module_table, ALL);
		queue_free(&st.module_table, sizeof(graph_element_t));
		return ret;
	}

	// Deregister old unused elements
	deregister_modules(st.module_table, UNMARKED);

	// Purge tokens, belonging to unmarked modules, from the mqueue.
	purge_tokens(st.module_table);

	// Reset the busy masks of old unused elements.
	reset_element_busy_mask(st.module_table);

	// The graph was initialized properly.
	// Check if the parameters need to be updated.
	if (update_format & PARAM_SECTION) {
		// Remove unmarked elements from elements table
		remove_unmarked_elements(&st.module_table);

		// Update the parameters list, and post a UPDATE_PARAMETERS task
		ret = update_parameter_table(update_format, st.module_table, cm, config_file_offset);
		if (ret < 0) {
			// Update parameter table failed. Do not apply any updates.
			// Free the module table.
			// The application will run with default/last set parameters.
			// Signal the base station about this error.
			queue_free(&st.module_table, sizeof(graph_element_t));
			return SOS_OK;
		}
		post_short(MULTICAST_SERV_PID, MULTICAST_SERV_PID, MSG_UPDATE_PARAMETERS, 0, 0, 0);
	} else if (update_format & MERGE_PARAMETERS) {
		post_short(MULTICAST_SERV_PID, MULTICAST_SERV_PID, MSG_UPDATE_PARAMETERS, 0, 0, 0);
	} else {
		// Free elements table in RAM
		queue_free(&st.module_table, sizeof(graph_element_t));
	}

	return ret;
}

static void load_elements_table(queue_header_t **module_table, uint8_t cmd)
{
	int8_t num_elements = 0, i = 0;
	size_t record_size = sizeof(graph_element_t) - sizeof(queue_header_t) - sizeof(uint8_t);
	size_t offset = ELEMENTS_TABLE_SKIP_OFFSET;

	// Return if elements table does not exist.
	if (st.elements_table == CODEMEM_INVALID) return;

	// If module table already exists, it may be a leak, or an older version.
	// So, destroy it before populating it afresh.
	if (*module_table != NULL) {
		queue_free(module_table, sizeof(graph_element_t));
	}

	// Read total number of elements in the table
	// The first byte of the elements table in flash contains this number
	ker_codemem_read(st.elements_table, MULTICAST_SERV_PID, &num_elements, sizeof(num_elements), 0);

	// Corrupted element table?
	if (num_elements < 0) return;

	// Read in all records 
	for (i = 0; i < num_elements; i++) {
		if (cmd == LOAD) {
			graph_element_t *elt = (graph_element_t *)vire_malloc(sizeof(graph_element_t), MULTICAST_SERV_PID);
			if (elt == NULL) {
				queue_free(module_table, sizeof(graph_element_t));
				*module_table = NULL;
				return;
			}
			ker_codemem_read(st.elements_table, MULTICAST_SERV_PID, &(elt->cid), record_size, offset);
			elt->marked = UNMARKED;
			queue_insert(module_table, (queue_header_t *)elt);
		} else if (cmd == DEREGISTER) {
			// Avoid using dynamic memory pool to read records from 
			// the elements table in flash
			graph_element_t elt;
			ker_codemem_read(st.elements_table, MULTICAST_SERV_PID, &(elt.cid), record_size, offset);
			invalidate_output_ports(elt.pid);
			ker_deregister_module(elt.pid);
			delete_module_header(elt.pid);
		} else {
			return;
		}
		offset += record_size;
	}
}

// For each output port u
//   If module(u) has not been discovered,
//     Spawn a new module, Store <code id, instance, module ID> in a table
//   Get the module header and patch GID corresponding to u
//   fanout = 0, routing_lst = NULL
//   For each input port v connected to u
//     If module(v) has not been discovered,
//      Spawn a new module, Store <code id, instance, module ID> in a table
//     Get the module header and patch the PID field in v
//     Get pointer to function control block corresponding to v
//     Get module ID of module(v)
//     Store <pointer [2], module ID [1]> in a linked list routing_lst
//     fanout++
//   Write <fanout [1], 0x0000 [2]> to codemem (input port table)
//   For each item in routing_lst
//     Write item [3] to codemem
//   Free routing_lst
//
// Store all module ID's of elements at the end of the input port table
// and store a reference (offset, length) to it. This is used for
// deregistration.
//
static int8_t interpret_wiring_config(codemem_t cm, size_t *file_offset, 
										uint16_t length, queue_header_t **module_table) {
	uint16_t flash_routing_table_offset = 0;
	uint16_t flash_elements_table_offset = 0;
	uint8_t current_gid = 0;
	uint16_t num_routing_entries = 0;
	uint8_t buf[ROUTING_TABLE_ENTRY_SIZE];
	queue_header_t *itr_el = NULL;

	// Allocate space for saving the current wiring table
	// if not already done so.
	uint16_t flash_wiring_table_offset = 0;
	if (st.saved_wiring_table == CODEMEM_INVALID) {
		st.saved_wiring_table = ker_codemem_alloc(length, CODEMEM_TYPE_EXECUTABLE);
		if (st.saved_wiring_table == CODEMEM_INVALID) return -ENOMEM;
	}

	while (1) {
		wiring_table_row_t record;
		sos_pid_t elementID;
		sos_module_t *element;
		uint32_t gid_phy_addr;
		uint8_t fanout;
		queue_header_t *routing_lst = NULL;
		uint8_t i;

		// Read the next record (OUTPUT port).
		// For each output record 'u',
		ker_codemem_read(cm, MULTICAST_SERV_PID, &record, sizeof(wiring_table_row_t),
									*file_offset);

		DEBUG("OUTPUT RECORD: %d %d %d %d %d\n", record.e.type, record.e.cid, 
						record.e.instance_id, record.port_id, record.gid_or_end);

		// Copy the record to the saved wiring table
		ker_codemem_write(st.saved_wiring_table, MULTICAST_SERV_PID, &record, 
							sizeof(wiring_table_row_t), flash_wiring_table_offset);
		flash_wiring_table_offset += sizeof(wiring_table_row_t);

		// It should be either an END_TABLE or parameter section header
		// or beginning of a new output group record.
		if ((record.e.type == END_TABLE) || (record.e.type == BEGIN_PARAMETERS)) {
			// The wiring configuration file has been read and processed.
			// Now break out of the loop and finish processing 
			// the graph elements table by writing it to flash for later use
			break;
		}

		if (record.e.type != OUTPUT_RECORD) {
			// There is some error in the wire configuration file.
			// Cleanup and return an error code.
			DEBUG("Expecting an OUTPUT record. But got %d type.", record.e.type);
			ker_codemem_free(st.saved_wiring_table);
			st.saved_wiring_table = CODEMEM_INVALID;
			return -EINVAL;
		}

		// Update pointer to wiring configuration file.
		(*file_offset) += sizeof(wiring_table_row_t);

		// Process module(u) to see if it has already been initialized or not.
		// If it was already initialized in a previous graph, its marked as
		// existing in new graph.
		// The module is properly initialized if it hadn't been done so, and
		// is added to the module_table.
		element = process_module((graph_element_t **)module_table, &record, &elementID);
		if (element == NULL) {
			DEBUG("Error while processing output module [%d, %d].\n", 
											record.e.cid, record.e.instance_id);
			ker_codemem_free(st.saved_wiring_table);
			st.saved_wiring_table = CODEMEM_INVALID;
			return -EINVAL;
		}
		DEBUG("Output module gets ID = %d.\n", elementID);

		// Get the actual physical address of the GID to be patched in the
		// source (output) module.
		gid_phy_addr = sos_get_physical_addr(element->header, 
							offsetof(mod_header_t, funct[record.port_id].fid));

		// Patch GID.
		ker_codemem_direct_write(gid_phy_addr, MULTICAST_SERV_PID, 
						&(current_gid), sizeof(current_gid), 0);

		// Read all the input ports connected to the output port above.
		routing_lst = NULL;
		fanout = 0;
		while (1) {
			wiring_table_row_t input_record;
			routing_table_elm_t *v;
			sos_module_t *input_element;

			// Read next record (INPUT port 'v').
			ker_codemem_read(cm, MULTICAST_SERV_PID, &input_record, sizeof(wiring_table_row_t),
								*file_offset);
			(*file_offset) += sizeof(wiring_table_row_t);

			DEBUG("INPUT: %d %d %d %d %d\n", input_record.e.type, input_record.e.cid, 
						input_record.e.instance_id, input_record.port_id, input_record.gid_or_end);

			// The record should be only be an INPUT RECORD.
			if (input_record.e.type != INPUT_RECORD) {
				// Signal an error and cleanup all saved state.
				DEBUG("Expecting an input record. But got %d type.\n", input_record.e.type);
				queue_free(&routing_lst, sizeof(routing_table_elm_t));
				ker_codemem_free(st.saved_wiring_table);
				st.saved_wiring_table = CODEMEM_INVALID;
				return -EINVAL;
			}

			// Keep a count of all the input ports connected to the output port
			// This becomes the first entry of the input port table stored 
			// in flash. It is read when a token is put on the respective output
			// port.
			fanout++;

			// Process module(v) to see if it has already been initialized or not.
			// The module is properly initialized if it hadn't been done so, and
			// is added to the init_module_table.
			input_element = process_module((graph_element_t **)module_table, &input_record, &elementID);
			if (input_element == NULL) {
				DEBUG("Error while processing input module [%d, %d].\n", 
										input_record.e.cid, input_record.e.instance_id);
				queue_free(&routing_lst, sizeof(routing_table_elm_t));
				ker_codemem_free(st.saved_wiring_table);
				st.saved_wiring_table = CODEMEM_INVALID;
				return -ENOMEM;
			}

			v = (routing_table_elm_t *)vire_malloc(sizeof(routing_table_elm_t), MULTICAST_SERV_PID);
			if (v == NULL) {
				DEBUG("Could not allocate RAM for routing table element.\n");
				queue_free(&routing_lst, sizeof(routing_table_elm_t));
				ker_codemem_free(st.saved_wiring_table);
				st.saved_wiring_table = CODEMEM_INVALID;
				return -ENOMEM;
			}

			// Get function control block corresponding to the input port.
			// Using fntable_real_subscribe() here automatically checks the 
			// types (proto) of output and input ports too. 
			// Element header is needed to get the prototype of the output
			// port.
			v->cb = fntable_real_subscribe(element->header, elementID, 
											input_record.port_id, record.port_id);
			if (v->cb == 0) {
				DEBUG("Couldn't find input function block for [%d, %d] port %d. ID %d.\n",
							input_record.e.cid, input_record.e.instance_id, input_record.port_id,elementID);
				queue_free(&routing_lst, sizeof(routing_table_elm_t));
				ker_codemem_free(st.saved_wiring_table);
				st.saved_wiring_table = CODEMEM_INVALID;
				return -EINVAL;
			}

			// Get [module ID | input port ID] of the input port.
			v->pid_port = (get_epid_from_pid(elementID) << RTABLE_INPUT_PORT_BITS) | 
								get_port_from_pid_port(input_record.port_id);

			// Add <input cb, module ID | input port> to the input port list connected to 'u'
			queue_insert(&routing_lst, (queue_header_t *)v);
			
			if (input_record.gid_or_end == END_RECORD) {
				// This is the last input port connected to output port 'u'.
				// Exit this loop and process remaining output ports.
				break;
			}
		}

		// Write the routing table generated for the output port 'u'
		// to flash.
		// First, write fanout to flash. The first byte of buffer holds
		// the fanout. Rest are empty to fill in one complete row
		// so that its easy to use GID as an index into this table.
		// Also increment current running GID by the fanout + 1.
		current_gid += (fanout + 1);
		buf[0] = fanout;
		for (i = 1; i < ROUTING_TABLE_ENTRY_SIZE; i++) {
			buf[i] = 0x00;
		}

		ker_codemem_write(st.routing_table, MULTICAST_SERV_PID, buf, 
							ROUTING_TABLE_ENTRY_SIZE, flash_routing_table_offset);
		flash_routing_table_offset += ROUTING_TABLE_ENTRY_SIZE;
		num_routing_entries++;

		// Next, write the routing table to flash and free the RAM occupied by it.
		while (routing_lst != NULL) {
			routing_table_elm_t *itr = (routing_table_elm_t *)routing_lst;
			routing_lst = routing_lst->next;
			ker_codemem_write(st.routing_table, MULTICAST_SERV_PID, 
								(uint8_t *)((uint8_t *)itr + sizeof(queue_header_t)), 
								ROUTING_TABLE_ENTRY_SIZE, flash_routing_table_offset);
			flash_routing_table_offset += ROUTING_TABLE_ENTRY_SIZE;
			vire_free(itr, sizeof(routing_table_elm_t));
			num_routing_entries++;
		}

		//ker_codemem_flush(st.routing_table, MULTICAST_SERV_PID);
	}

	// We have finished processing the whole wiring configuration file.

	// Write the list of marked elements along with their pid's to flash.
	// This list is used while deregistering and removing those elements
	// when the graph is modified or removed.
	// The first byte of the module table stores the number of elements
	// in the graph.
	itr_el = *module_table;
	while (itr_el != NULL) {
		if (((graph_element_t *)itr_el)->marked == MARKED) {
			st.num_elements++;
		}
		itr_el = itr_el->next;
	}
	ker_codemem_write(st.elements_table, MULTICAST_SERV_PID, &st.num_elements, sizeof(uint8_t), 0);
	flash_elements_table_offset++;

	itr_el = *module_table;
	while (itr_el != NULL) {
		uint16_t nbytes = sizeof(sos_code_id_t) + sizeof(uint8_t) + sizeof(sos_pid_t);
		graph_element_t *e = (graph_element_t *)itr_el;
		if (e->marked == MARKED) {
			ker_codemem_write(st.elements_table, MULTICAST_SERV_PID, 
						(void *)((uint8_t *)itr_el + sizeof(queue_header_t)), 
						nbytes, flash_elements_table_offset);
			flash_elements_table_offset += nbytes;
		}
		itr_el = itr_el->next;
	}

	ker_codemem_flush(st.elements_table, MULTICAST_SERV_PID);

	if (load_routing_table(&st.routing_table_ram, st.routing_table, num_routing_entries) < 0) {
		// There is no space in RAM to hold the routing table.
		// So, use the flash table only.
		st.routing_table_ram = NULL;
	}

	return SOS_OK;

}

static int8_t update_parameter_table(uint8_t update_format, queue_header_t *module_table, 
												codemem_t cm, size_t *file_offset) {
	section_header_t sec;

	// merger holds the new parameter configuration.
	// Finally, it is copied onto st.saved_params_table.
	codemem_t merger;
	// Save space in merged file for writing final length in the end.
	uint16_t merger_offset = sizeof(uint16_t);

	// Read the section header of wiring config file to ensure correctness of file
	// and obtain length of parameter update.
	ker_codemem_read(cm, MULTICAST_SERV_PID, &sec, sizeof(section_header_t), *file_offset);

	if ((sec.type != BEGIN_PARAMETERS) || (sec.length <= 0)) {
		DEBUG("Expecting section header with length > 0. Corrupted packet.\n");
		return -EINVAL;
	}

	(*file_offset) += sizeof(section_header_t);

	if ((update_format & MERGE_PARAMETERS) && (st.saved_params_table != CODEMEM_INVALID)) {
		// Copy parameters of existing elements into merger if they
		// haven't been updated.
		param_table_row_t saved_row;
		uint16_t length;
		size_t saved_offset = 0;

		// Get length of saved parameters table
		ker_codemem_read(st.saved_params_table, MULTICAST_SERV_PID, &length, 
												sizeof(length), saved_offset); 
		saved_offset += sizeof(length);

		// Allocate flash page to hold merged parameter configuration.
		merger = ker_codemem_alloc(length+sec.length, CODEMEM_TYPE_EXECUTABLE);

		// For all elements in the saved parameter configuration file
		while (1) {
			uint8_t exist_cond;
			sos_pid_t elementID;

			// Read first saved record.
			ker_codemem_read(st.saved_params_table, MULTICAST_SERV_PID, &saved_row, 
										sizeof(param_table_row_t), saved_offset);
			saved_offset += sizeof(param_table_row_t);

			// Exit loop if its an END_TABLE record.
			if (saved_row.e.type == END_TABLE) {
				break;
			}

			// Check if this element exists in current configuration.
			exist_cond = discovered((graph_element_t *)module_table, &saved_row.e, &elementID);

			// If it does, then check for updates in wiring configuration file.
			if (exist_cond == THIS_INSTANCE_EXISTS) {
				// Check if updated parameters are available for this element.
				if (!check_updated_params_for_element(cm, *file_offset, saved_row)) {
					// The element does not have updated parameters.
					// Copy the old parameters into merger.
					uint8_t *params = (uint8_t *)vire_malloc(saved_row.length, MULTICAST_SERV_PID);
					if (params == NULL) {
						// No more memory to move parameters to merger.
						ker_codemem_free(merger);
						return -ENOMEM;
					}
					ker_codemem_read(st.saved_params_table, MULTICAST_SERV_PID, params, 
																saved_row.length, saved_offset);
					ker_codemem_write(merger, MULTICAST_SERV_PID, &saved_row, 
											sizeof(param_table_row_t), merger_offset);
					merger_offset += sizeof(param_table_row_t);

					// Write params to merger.
					ker_codemem_write(merger, MULTICAST_SERV_PID, params, 
											saved_row.length, merger_offset);
					merger_offset += saved_row.length;

					// params is no longer needed. Free it.
					vire_free(params, saved_row.length);
				} 
			}
			// This element does not exist any more or has updated parameters.
			// Discard this record.
			// Advance saved_offset to next record.
			saved_offset += saved_row.length;
		}
	} else {
		// Either required to discard old parameters, or there are no
		// previously saved parameters.
		// Allocate flash page to hold latest parameter configuration.
		merger = ker_codemem_alloc(sec.length, CODEMEM_TYPE_EXECUTABLE);
	}

	// Copy the updated parameters onto merger.
	// For all records in update
	while (1) {
		param_table_row_t new_row;
		uint8_t *params;

		// Read record.
		ker_codemem_read(cm, MULTICAST_SERV_PID, &new_row, sizeof(param_table_row_t), *file_offset);

		// If its END_TABLE record, then exit loop.
		if (new_row.e.type == END_TABLE) {
			int8_t ndummy_bytes = (int8_t)(sizeof(section_header_t) - sizeof(param_table_row_t));
			// Write the end table record to mark the end of merger.
			ker_codemem_write(merger, MULTICAST_SERV_PID, &new_row, 
										sizeof(param_table_row_t), merger_offset);
			merger_offset += sizeof(param_table_row_t);
			// Add 'n' dummy bytes to make end section marker the same length as
			// other section markers.
			if (ndummy_bytes > 0) {
				uint8_t buf[ndummy_bytes];
				ker_codemem_write(merger, MULTICAST_SERV_PID, buf, ndummy_bytes, merger_offset);
				merger_offset += ndummy_bytes;
			}

			// Write the total number of bytes at the beginning of merger.
			ker_codemem_write(merger, MULTICAST_SERV_PID, &merger_offset, 
										sizeof(merger_offset), 0);
			break;
		}

		// If it is a corrupted record, return error.
		if (new_row.e.type != PARAMETER_RECORD) {
			ker_codemem_free(merger);
			return -EINVAL;
		}
		
		// Read parameter values into params.
		(*file_offset) += sizeof(param_table_row_t);
		params = (uint8_t *)vire_malloc(new_row.length, MULTICAST_SERV_PID);
		if (params == NULL) {
			// No more memory to move parameters to merger.
			ker_codemem_free(merger);
			return -ENOMEM;
		}
		ker_codemem_read(cm, MULTICAST_SERV_PID, params, new_row.length, *file_offset);
		(*file_offset) += new_row.length;

		// Write the complete record into merger.
		ker_codemem_write(merger, MULTICAST_SERV_PID, &new_row, 
							sizeof(param_table_row_t), merger_offset);
		merger_offset += sizeof(param_table_row_t);

		ker_codemem_write(merger, MULTICAST_SERV_PID, params, 
									new_row.length, merger_offset);
		merger_offset += new_row.length;

		// params is no longer needed. Free it.
		vire_free(params, new_row.length);
	}

	// st.saved_params_table now points to merger.
	// Free old table.
	ker_codemem_free(st.saved_params_table);
	st.saved_params_table = merger;

	return SOS_OK;

}

static bool check_updated_params_for_element(codemem_t cm, size_t file_offset, param_table_row_t elm) {
	param_table_row_t row;

	while (1) {
		ker_codemem_read(cm, MULTICAST_SERV_PID, &row, sizeof(param_table_row_t), file_offset);
		if ((row.e.type == END_TABLE) || (row.e.type != PARAMETER_RECORD)) break;
		file_offset += (sizeof(param_table_row_t) + row.length);
		if ((row.e.cid == elm.e.cid) && (row.e.instance_id == elm.e.instance_id)) return true;
	}

	return false;
}


static int8_t apply_parameter_update(mcast_dyn_fn_state_t *s) {
	size_t offset = PARAM_TABLE_SKIP_OFFSET;

	// Signal error if saved parameter table doesnt exist.
	if (st.saved_params_table == CODEMEM_INVALID) return -EINVAL;
	
	// If module table is not loaded in RAM, read it from flash.
	if (st.module_table == NULL) {
		load_elements_table(&st.module_table, LOAD);
		if (st.module_table == NULL) return -EINVAL;
	}

	// For all parameter records
	while(1) {
		param_table_row_t row;
		sos_pid_t elementID = NULL_PID;
		uint8_t exist_cond;
		int8_t ret;
		void *params;

		// Read the record from flash
		ker_codemem_read(st.saved_params_table, MULTICAST_SERV_PID, &row, 
								sizeof(param_table_row_t), offset);
		offset += sizeof(param_table_row_t);

		// Parameter update successfully finished if encounter END_TABLE record.
		if (row.e.type == END_TABLE) break;

		// Corrupted parameter table. Signal error.
		if (row.e.type != PARAMETER_RECORD) return -EINVAL;

		// Check if the element exists in current graph.
		// elementID is updated automatically.
		exist_cond = discovered((graph_element_t *)st.module_table, &row.e, &elementID);

		// If the element doesn't exist, ignore the record.
		if (exist_cond != THIS_INSTANCE_EXISTS) {
			offset += row.length;
			continue;
		}

		// Subscribe to the update_param function provided by that element.
		ret = ker_fntable_subscribe(MULTICAST_SERV_PID, elementID, UPDATE_PARAM_FID, 0);

		// Subscription failed. Move onto next record.
		if (ret < 0) {
			offset += row.length;
			continue;
		}

		params = vire_malloc(row.length, MULTICAST_SERV_PID);
		// No more memory to apply parameter updates. Signal error.
		if (params == NULL) return -ENOMEM;

		// Read parameters from flash.
		ker_codemem_read(st.saved_params_table, MULTICAST_SERV_PID, params, row.length, offset);
		offset += row.length;

		// Apply update.
		SOS_CALL(s->update_param, update_param_func_t, params, row.length);

		vire_free(params, row.length);
	}

	return SOS_OK;
}

static sos_module_t *process_module(graph_element_t **init_module_table, wiring_table_row_t *record, 
																sos_pid_t *elementID) {
	uint8_t exist_cond = discovered(*init_module_table, &(record->e), elementID);
	mod_header_ptr element_header = 0;
	sos_module_t *element = NULL;

	if (exist_cond != THIS_INSTANCE_EXISTS) {
		graph_element_t *new;
		uint8_t num_sub_func, num_prov_func, num_out_port;
		// Spawn a new instance of the module using original header
		if (exist_cond == NO_INSTANCE_EXISTS) {
			element_header = ker_codemem_get_header_from_code_id(record->e.cid);
			*elementID = ker_spawn_module(element_header, NULL, 0, SOS_CREATE_THREAD);
		} else if (exist_cond == ANOTHER_INSTANCE_EXISTS) {
			// Copy the original header and spawn an instance using the header copy
			mod_header_ptr hdr_copy = copy_module_header(record->e.cid);
			*elementID = ker_spawn_module(hdr_copy, NULL, 0, SOS_CREATE_THREAD);
			update_last_module_added(*elementID);
		}

		// For some reason, module could not be spawned. Signal error.
		if (*elementID == NULL_PID) return NULL;

		new = vire_malloc(sizeof(graph_element_t), MULTICAST_SERV_PID);
		if (new == NULL) {
			// Module may already have been spawned.
			// Deregister the module here as it hasn't been added
			// to the list yet.
			ker_deregister_module(*elementID);
			return NULL;
		}
		new->cid = record->e.cid;
		new->instance_id = record->e.instance_id;
		new->pid = *elementID;
		new->marked = MARKED;
		queue_insert((queue_header_t **)init_module_table, (queue_header_t *)new);
		
		// Patch the PID field of all published functions
		// so as to enable correct tracking of PID's.
		element = ker_get_module(*elementID);
		if (element == NULL) {
			DEBUG("Unable to get module header for ID %d.\n", *elementID);
			return NULL;
		}
		num_out_port = sos_read_header_byte(element->header, 
							offsetof(mod_header_t, num_out_port));
		num_sub_func = sos_read_header_byte(element->header, 
							offsetof(mod_header_t, num_sub_func));
		num_prov_func = sos_read_header_byte(element->header, 
							offsetof(mod_header_t, num_prov_func));

		// Patch the PID's of all published functions with elementID
		patch_all_published_functions(element->header, *elementID, num_sub_func, 
													num_sub_func + num_prov_func);
		// Patch the PID's of all output ports with element ID
		patch_all_published_functions(element->header, *elementID, 0, num_out_port);
	} else {
		element = ker_get_module(*elementID);
	}

	return element;
}

static void patch_all_published_functions(mod_header_ptr header, sos_pid_t pid, 
											uint8_t start, uint8_t end) {
	uint8_t i;

	for (i = start; i < end; i++) {
		uint32_t phy_addr = sos_get_physical_addr(header, 
								offsetof(mod_header_t, funct[i].pid));
		ker_codemem_direct_write(phy_addr, MULTICAST_SERV_PID, &pid, sizeof(sos_pid_t), 0);
	}
}

static void invalidate_output_ports(sos_pid_t pid) {
	uint8_t i, fid = INVALID_GID;
	sos_module_t *element = ker_get_module(pid);
	uint8_t num_out_port;

	num_out_port = sos_read_header_byte(element->header, 
						offsetof(mod_header_t, num_out_port));
	
	for (i = 0; i < num_out_port; i++) {
		uint32_t phy_addr = sos_get_physical_addr(element->header, 
								offsetof(mod_header_t, funct[i].fid));
		ker_codemem_direct_write(phy_addr, MULTICAST_SERV_PID, &fid, sizeof(uint8_t), 0);
	}
}

static void queue_insert(queue_header_t **head, queue_header_t *elm) {
	if (*head == NULL) {
		*head = elm;
	} else {
		queue_header_t *itr = *head;
		while (itr->next != NULL)
			itr = itr->next;
		itr->next = elm;
	}
	elm->next = NULL;
}

static void queue_remove(queue_header_t **head, queue_header_t *elm, uint16_t size) {
	queue_header_t *itr = *head;

	if ((itr == NULL) || (elm == NULL)) return;
	
	if (itr == elm) {
		*head = elm->next;
		vire_free(elm, size);
		return;
	}

	while ((itr != NULL) && (itr->next != elm)) {
		itr = itr->next;
	}
	if (itr == NULL) return;
	itr->next = elm->next;
	vire_free(elm, size);
}

static void queue_free(queue_header_t **head, uint16_t size) {
	while (*head != NULL) {
		queue_header_t *itr = *head;
		*head = (*head)->next;
		vire_free(itr, size);
	}
}

static mesg_queue_t *mesg_queue_insert(mesg_queue_t **head, uint8_t epid) {
	// Find the head of token queue for element 'epid'
	mesg_queue_t *itr = search_mesg_queue(*head, epid);

	// If the element already exists, return it.
	if (itr != NULL) return itr;

	// Otherwise allocate space for it and insert it into the message queue.
	itr = (mesg_queue_t *)vire_malloc(sizeof(mesg_queue_t), MULTICAST_SERV_PID);
	if (itr == NULL) return NULL;

	itr->tokens = NULL;
	itr->epid = epid;
	queue_insert((queue_header_t **)head, (queue_header_t *)itr);

	return itr;

}

static void token_queue_remove(mesg_queue_t **head, uint8_t epid, queue_header_t *elm) {
	// Find the head of token queue for element 'epid'
	mesg_queue_t *itr = search_mesg_queue(*head, epid);

	// Token queue not found. Return.
	if ((itr == NULL) || (elm == NULL)) return;

	// Token queue found. Remove the element 'elm' from it.
	queue_remove((queue_header_t **)&itr->tokens, elm, sizeof(token_queue_t));

	// If no more tokens, remove the head of token queue
	// for element 'epid'
	if (itr->tokens == NULL) {
		queue_remove((queue_header_t **)head, (queue_header_t *)itr, sizeof(mesg_queue_t));
	}

}

static void token_queue_free(token_queue_t **head) {
	// Free all the tokens in the token queue pointed 
	// to by head.
	while (*head != NULL) {
		token_queue_t *del = *head;
		*head = (token_queue_t *)((*head)->h.next);
		destroy_token(del->t);
		vire_free(del, sizeof(token_queue_t));
	}
}

static void mesg_queue_free(mesg_queue_t **head) {
	while (*head != NULL) {
		mesg_queue_t *del = *head;
		*head = (mesg_queue_t *)((*head)->h.next);
		token_queue_free(&del->tokens);
		vire_free(del, sizeof(mesg_queue_t));
	}
}

static mesg_queue_t *search_mesg_queue(mesg_queue_t *head, sos_pid_t epid) {
	mesg_queue_t *itr = head;

	// Find the head of token queue for element 'epid'
	while ((itr != NULL) && (itr->epid != epid)) {
		itr = (mesg_queue_t *)(itr->h.next);
	}

	return itr;
}

static bool tokens_posted_for_element(sos_pid_t epid) {
	mesg_queue_t *head = search_mesg_queue(st.mqueue, epid);

	if (head == NULL) return false;

	token_queue_t *itr = head->tokens;
	while (itr != NULL) {
		if (itr->status == TOKEN_POSTED) return true;
		itr = (token_queue_t *)(itr->h.next);
	}

	return false;
}

static void purge_tokens(queue_header_t *module_table) {
	graph_element_t *itr = (graph_element_t *)module_table;

	// For all UNMARKED elements
	while (itr != NULL) {
		if (itr->marked == UNMARKED) {
			// Get the head of the token queue, if it exists
			mesg_queue_t *del = search_mesg_queue(st.mqueue, get_epid_from_pid(itr->pid));
			if (del != NULL) {
				// Free the token queue
				token_queue_free(&del->tokens);
				// Remove the head from the message queue
				queue_remove((queue_header_t **)&st.mqueue, (queue_header_t *)del, sizeof(mesg_queue_t));
			}
		}
		itr = (graph_element_t *)(itr->h.next);
	}
}

static void deregister_modules(queue_header_t *mod_table, uint8_t cmd) {
	graph_element_t *itr = (graph_element_t *)mod_table;

	while (itr != NULL) {
		uint8_t del = 0;
		switch(cmd) {
			case UNMARKED:
			{
				if (itr->marked == UNMARKED) del = 1;
				break;
			}
			case MARKED:
			{
				if (itr->marked == MARKED) del = 1;
				break;
			}
			case ALL:
			{
				del = 1;
				break;
			}
			default: break;
		}
		if (del) {
			invalidate_output_ports(itr->pid);
			ker_deregister_module(itr->pid);
			delete_module_header(itr->pid);
		}
		itr = (graph_element_t *)itr->h.next;
	}
}

static void remove_unmarked_elements(queue_header_t **module_table) {
	graph_element_t *itr = (graph_element_t *)(*module_table);

	while (itr != NULL) {
		graph_element_t *del = itr;
		if (itr->marked == UNMARKED) {
			queue_remove(module_table, (queue_header_t *)del, sizeof(graph_element_t));
		}
		itr = (graph_element_t *)(itr->h.next);
	}
}

static uint8_t discovered(graph_element_t *head, common_table_header_t *row, sos_pid_t *modID) {
	graph_element_t *itr = head;
	uint8_t ret = NO_INSTANCE_EXISTS;
	*modID = 0;

	while (itr != NULL) {
		if (itr->cid == row->cid) {
			ret = ANOTHER_INSTANCE_EXISTS;
			if (itr->instance_id == row->instance_id) {
				*modID = itr->pid;
				itr->marked = MARKED;
				return THIS_INSTANCE_EXISTS;
			}
		}
		itr = (graph_element_t *)(itr->h.next);
	}
	
	return ret;
}

static bool is_graph_busy() {
	uint8_t i, j;

	for (i = 0; i < MAX_NUM_ELEMENTS; i++) {
		for (j = 0; j < (MAX_INPUT_PORTS_PER_ELEMENT+7)/8; j++) {
			if (~(st.busy_bit_mask[i][j]) > 0) return true;
		}
	}

	return false;
}

static bool is_element_ready(uint8_t pid_port) {
	uint8_t pid_index = get_epid_from_pid_port(pid_port);
	uint8_t port_bit_position = get_port_from_pid_port(pid_port);
	uint8_t shift = port_bit_position % 8;
	bool ret = ((st.busy_bit_mask[pid_index][port_bit_position / 8] << shift) & 0x80) > 0 ? true : false;
	return ret;
}

static void set_element_status(sos_pid_t epid, uint8_t portID, uint8_t status) {
	uint8_t shift = portID % 8;
	
	if (status == 0) {
		st.busy_bit_mask[epid][portID / 8] &= ~( 0x80 >> shift);
	} else {
		// Set element as READY: All input ports are ready.
		memset(&(st.busy_bit_mask[epid][0]), 0xFF, (MAX_INPUT_PORTS_PER_ELEMENT+7)/8);
		//st.busy_bit_mask[epid][port_bit_position / 8] |= ( 0x80 >> shift);
	}
}

static void	reset_element_busy_mask(queue_header_t *module_table) {
	graph_element_t *itr = (graph_element_t *)module_table;

	while (itr != NULL) {
		if (itr->marked == UNMARKED) {
			set_element_status(get_epid_from_pid(itr->pid), 0, ELEMENT_READY);
		}
		itr = (graph_element_t *)(itr->h.next);
	}
}

static void post_tokens_for_all_ports(sos_pid_t epid) {
	// Search for head of token queue for element 'epid'
	mesg_queue_t *head = search_mesg_queue(st.mqueue, epid);
	token_queue_t *itr;

	if (head == NULL) return;

	itr = head->tokens;
	while (itr != NULL) {
		uint8_t pid_port = (epid << RTABLE_INPUT_PORT_BITS) | get_port_from_pid_port(itr->portID);
		if (is_element_ready(pid_port) && (itr->status == TOKEN_QUEUED)) {
			msg_continue_t *m = (msg_continue_t *)vire_malloc(sizeof(msg_continue_t), MULTICAST_SERV_PID);
			if (m == NULL) {
				DEBUG("COULDN'T ALLOCATE NEXT TOKEN %d: MEMORY FULL\n", *((uint8_t*)itr->t->data));
				return;
			}
			m->mptr = itr;
			m->epid = epid;
			if (post_long(MULTICAST_SERV_PID, MULTICAST_SERV_PID, MSG_CONTINUE_TASK, 
						sizeof(msg_continue_t), m, SOS_MSG_RELEASE | SOS_MSG_HIGH_PRIORITY) != SOS_OK) {
				DEBUG("COULDN'T POST NEXT TOKEN %d: MEMORY FULL\n", *((uint8_t*)itr->t->data));
				return;
			}
			DEBUG("Found token %d in the queue for element %d.\n", *((uint8_t*)itr->t->data), epid);
			// Set the status to BUSY again so that the new tokens always go
			// into the token queue.
			set_element_status(epid, get_port_from_pid_port(itr->portID), ELEMENT_BUSY);
			itr->status = TOKEN_POSTED;
		}
		itr = (token_queue_t *)(itr->h.next);
	}
}

//----------------------------------------------------
// Functions published by the engine.
//----------------------------------------------------

// This function should be REENTRANT.
// Make sure that this property is always maintained.
//static int8_t dispatch(func_cb_ptr p, func_cb_ptr caller_cb, token_type_t *t) {
int8_t dispatch(func_cb_ptr caller_cb, token_type_t *t) {
	// Fetch GID, module ID from caller_cb
	uint8_t gid = sos_read_header_byte(caller_cb, offsetof(func_cb_t, fid));
	
	// Caller PID is used to reset the BUSY status if previously set, and
	// dispatch the waiting tokens, if any.
	uint8_t caller_pid = sos_read_header_byte(caller_cb, offsetof(func_cb_t, pid));

	// Get 'n' from the input port table
	uint8_t num_connected_ports, i;
	routing_table_ram_t port;

	if (gid == INVALID_GID) goto set_element_ready;

	DEBUG("Dispatch called in Engine. Caller PID = %d. GID = %d. \n", caller_pid, gid);
	
	//read_routing_table(st.routing_table, st.routing_table_ram, gid, &port);
	//num_connected_ports = port.index.num_ports;
#ifdef PUT_ROUTING_TABLE_IN_RAM
	num_connected_ports = st.routing_table_ram[gid].index.num_ports;
#else
	ker_codemem_read(st.routing_table, MULTICAST_SERV_PID, &num_connected_ports, 
						sizeof(num_connected_ports), ROUTING_TABLE_ENTRY_SIZE * gid);
#endif

	// For 0 .. n
	// 	Get input CB
	// 	Call the function
	for (i = 1; i <= num_connected_ports; i++) {
		//read_routing_table(st.routing_table, st.routing_table_ram, gid+i, &port);
#ifdef PUT_ROUTING_TABLE_IN_RAM
		port.index.pid_port = st.routing_table_ram[gid+i].index.pid_port;
		port.cb = st.routing_table_ram[gid+i].cb;
#else
		ker_codemem_read(st.routing_table, MULTICAST_SERV_PID, &port, 
					ROUTING_TABLE_ENTRY_SIZE, ROUTING_TABLE_ENTRY_SIZE * (gid + i));
#endif
		if (is_element_ready(port.index.pid_port)) {
			int8_t status = SOS_CALL(port.cb, input_func_t, t);
			if (status == -EBUSY) {
				// Element has accepted the current input token
				// and will be performing a long (split-phase) operation on it.
				// Set its bit mask to indicate ELEMENT_BUSY status for future
				// tokens.
				set_element_status(get_epid_from_pid_port(port.index.pid_port), 
									get_port_from_pid_port(port.index.pid_port), ELEMENT_BUSY);
			} else if (status < 0) {
				// Some other error occured.
				// Stop the application graph.
				// Signal error to the base station.
				// Post a message (RESET task, HIGHEST priority) to itself.
				//graph_reset();
				return -EINVAL;
				//break;
			}
		} else {
			// Element has already indicated busy status.
			// Copy and enqueue this token.
			token_queue_t *new = (token_queue_t *)vire_malloc(sizeof(token_queue_t), MULTICAST_SERV_PID); 
			mesg_queue_t *tq = NULL;
			void *token_data = NULL;
			if (new == NULL) {
				// Stop the application graph.
				// Signal error to the base station.
				DEBUG("\n");
				DEBUG("TOKEN DROPPED: No more space for token %d.\n", *((uint8_t*)t->data));
				DEBUG("\n");
				continue;
				//queue_free(&st.mqueue);
			}
			token_data = capture_token_data(t, MULTICAST_SERV_PID);
			new->t = create_token(token_data, t->length, MULTICAST_SERV_PID);
			if (new->t == NULL) {
				// Stop the application graph.
				// Signal error to the base station.
				DEBUG("\n");
				DEBUG("TOKEN DROPPED: No more space for token %d.\n", *((uint8_t*)t->data));
				DEBUG("\n");
				destroy_token_data(token_data, t->type, t->length);
				vire_free(new, sizeof(token_queue_t));
				continue;
				//queue_free(&st.mqueue);
			}
			new->cb = port.cb;
			new->portID = get_port_from_pid_port(port.index.pid_port);
			new->status = TOKEN_QUEUED;
			tq = mesg_queue_insert(&st.mqueue, get_epid_from_pid_port(port.index.pid_port));
			if (tq == NULL) {
				// Stop the application graph.
				// Signal error to the base station.
				DEBUG("\n");
				DEBUG("TOKEN DROPPED: No more space for token %d.\n", *((uint8_t*)t->data));
				DEBUG("\n");
				destroy_token(new->t);
				vire_free(new, sizeof(token_queue_t));
				continue;
			}
			queue_insert((queue_header_t **)&tq->tokens, (queue_header_t *)new);
			DEBUG("Destination element is BUSY. Token %d queued.\n", *((uint8_t*)t->data));
		}
	}

	//If there are already tokens posted for the element,
	//then probably placing token on another output port set it READY,
	//or, there are some tokens waiting for other ports.
	//In this case, do not set the element status as READY.
set_element_ready:
	if (!tokens_posted_for_element(get_epid_from_pid(caller_pid))) {
		// Set the caller element status to be READY
		// All input ports are set to READY, so portID = x.
		set_element_status(get_epid_from_pid(caller_pid), 0, ELEMENT_READY);

		// Check if there are any queued tokens for the caller element.
		// Post the tokens (CONTINUE task) for different ports to itself.
		post_tokens_for_all_ports(get_epid_from_pid(caller_pid));
	}
	return SOS_OK;
}

static int8_t signal_error(func_cb_ptr p, int8_t error) { return SOS_OK; }

/*
void read_routing_table_in_flash(codemem_t routing_table, uint16_t gid, routing_table_ram_t *ptr) {
	ker_codemem_read(routing_table, MULTICAST_SERV_PID, ptr, 
						ROUTING_TABLE_ENTRY_SIZE, ROUTING_TABLE_ENTRY_SIZE * gid);
}
*/

#ifdef PUT_ROUTING_TABLE_IN_RAM
static int8_t load_routing_table(routing_table_ram_t **tab, codemem_t cm, uint16_t num_entries) {
	uint16_t i;

	*tab = (routing_table_ram_t *)ker_malloc(num_entries*sizeof(routing_table_ram_t), 
																	MULTICAST_SERV_PID);

	if (*tab == NULL) return -ENOMEM;

	for (i = 0; i < num_entries; i++) {
		ker_codemem_read(cm, MULTICAST_SERV_PID, &((*tab)[i]), ROUTING_TABLE_ENTRY_SIZE, 
														ROUTING_TABLE_ENTRY_SIZE * i);

	}

	return SOS_OK;
}

/*
static void read_routing_table_in_ram(codemem_t cm, routing_table_ram_t *tab, uint16_t gid, routing_table_ram_t *ptr) {
	if (tab != NULL) {
		ptr->index.pid_port = tab[gid].index.pid_port;
		ptr->cb = tab[gid].cb;
	} else {
		read_routing_table_in_flash(cm, gid, ptr);
	}
}
*/

static inline void reset_routing_table_in_ram() {
	ker_free(st.routing_table_ram);
	st.routing_table_ram = NULL;
}

#endif


#ifndef _MODULE_
mod_header_ptr wiring_engine_get_header() { 
	return sos_get_header_address(mod_header); 
} 
#endif

