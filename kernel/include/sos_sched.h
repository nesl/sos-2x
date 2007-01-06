
#ifndef _SOS_SCHED_H_
#define _SOS_SCHED_H_
#include <hardware_types.h>
#include <sos_types.h>
#include <sos_module_types.h>

enum {
	SCHED_NUMBER_BINS    = 4,      //!< number of bins to store modules
};


// For software based interrupt called by scheduler
typedef void (*sched_int_t)(void);
enum {
	SCHED_NUM_INTS           = 4,
	SCHED_TIMER_INT          = 0,	
	SCHED_UART_SEND_INT      = 1,
	SCHED_UART_RECV_INT      = 2,
	SCHED_I2C_INT            = 3,	
};

/**
 * @brief schedule a callback to handle hardware interrupt
 * 
 * sched_add_interrupt adds a callback to scheduler.
 * The callback will be call when scheduler has gained the control of 
 * CPU.  
 * @warning add_interrupt requires id.  Each kernel module that need to 
 * use this service should reserve *unique* id above.  Also make sure to 
 * increase SCHED_NUM_INTS
 */
extern void sched_add_interrupt(uint8_t id, sched_int_t f);

/**
 * @brief scheduler initialization
 * @param cond system condition during initialization
 */
extern void sched_init(uint8_t cond);

/**
 * @brief main scheduler routine, it will never return!
 */
extern void sched(void);

/**
 * @brief register a new module
 * @param h pointer to module header, must use sos_get_header_address macro
 * @return errno
 *
 * register task for message handling
 */
extern int8_t ker_register_module(mod_header_ptr h);

/**
 * @brief de-register a task (module)
 * @param pid task id to be removed
 *
 * NOTE THAT THIS FUNCTION CANNOT BE CALLED IN INTERRUPT HANDLER
 */
extern int8_t ker_deregister_module(sos_pid_t pid);


/**
 * @brief spawn new module
 * @param h     pointer to module header, must use sos_get_header_address macro
 * @param init  data to be pass into MSG_INIT (has to be from ker_malloc())
 * @param size  the size of init
 * @param flag  init_flag: can be either 0 or SOS_CREATE_THREAD
 * @return sos_pid_t when success, NULL_PID when failed
 *
 * 0 will register module with module ID
 * SOS_CREATE_THREAD will register module with internal ID allocated from ID pool
 */
extern sos_pid_t  ker_spawn_module(mod_header_ptr h,
		void *init, uint8_t init_size, uint8_t flag);

/**
 * @brief get pointer to module data structure from pid
 * @return handle if successful, NULL otherwise
 */
extern sos_module_t* ker_get_module(sos_pid_t pid);

/**
 * @brief get pointer to module's state
 * @return pointer to module's state
 */
extern void* ker_get_module_state(sos_pid_t pid);

/**
 * @brief kill all instances of modules with code_id
 */
extern void ker_killall(sos_code_id_t code_id);
/**
 * @brief get all modules
 *
 * modules are organized in the hash table with number of bins defined as
 * SCHED_NUMBER_BINS
 */
extern sos_module_t **sched_get_all_module(void);

/**
 * @brief set current executing module ID
 * @warning this is only to be used by function pointer jump tables
 * @return previous executing module ID
 */
extern sos_pid_t ker_set_current_pid( sos_pid_t pid );

/**
 * @brief get current executing module ID
 * @return executing module ID
 */
extern sos_pid_t ker_get_current_pid( void );
/**                                                           
 * @brief Message filtering rules interface                   
 * @param rules  new rule                                     
 */                                                           

extern int8_t ker_msg_change_rules(sos_pid_t sid, uint8_t rules);

/**
 * @brief get message rules from scheduler
 * @param pid the module id of rules interested
 * @param rules pointer to the rules
 * @return 0 for success, -EINVAL for fail
 */
extern int8_t sched_get_msg_rule(sos_pid_t pid, sos_ker_flag_t *rules);

/**
 * @brief put message to scheduler queue
 * this function always succeed
 */
extern void sched_msg_alloc(Message *m);

/**
 * @brief remove message from scheduler queue
 */
extern void sched_msg_remove(Message *m);

/**
 * @brief post crash check up
 * CALLED BY hardware.c of each platform
 */
extern void sched_post_crash_checkup(void);

/**
 * @brief dispatch short message 
 * This is used by the callback that was register by interrupt handler
 */
extern void sched_dispatch_short_message(sos_pid_t dst, sos_pid_t src,
		uint8_t type, uint8_t byte,
		uint16_t word, uint16_t flag);


/**
 * @brief register a static kernel model
 *
 * Instead of dynamic allocate memory, static module can just pass in
 * static allocated memory as state_ptr
 * @param handle pointer to sos_module_t, kernel module needs to provide storage of module data structure
 * @param h pointer to module header, must use sos_get_header_address macro
 * @param state_ptr pointer to module state
 */
extern int8_t sched_register_kernel_module(sos_module_t *handle, mod_header_ptr h, void *state_ptr);

extern uint8_t sched_stalled;

extern sos_pid_t    curr_pid;                      //!< current executing pid
extern sos_pid_t*   pid_sp;                        //!< pid stack pointer


#define SCHED_STALL() {sched_stalled = true;}

#define SCHED_RESUME() {sched_stalled = false;}

#endif

