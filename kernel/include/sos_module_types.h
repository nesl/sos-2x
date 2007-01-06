
#ifndef _SOS_MODULE_TYPES_H
#define _SOS_MODULE_TYPES_H
#include <fntable_types.h>
#include <message_types.h>
#include <hardware_types.h>

typedef uint16_t sos_code_id_t;
/**
 * @brief module header
 *
 * NOTE: The header is stored in read only memory
 * Special access functions are needed to access the header
 * use sos_read_header_byte(<byte field>) to access byte field
 * use sos_read_header_word(<word field>) to access 2-bytes field
 * use sos_read_header_ptr(<ptr> field>) to access pointer field (i.e. module_handler)
 *
 * NOTE: access read only memory is slower than RAM access.
 * So please avoid using sos_read_header_* in the loop
 * NOTE: when modifying this structure, please keep it 16 bits align.
 * NOTE: MSP430-GCC does not permit function pointers in a packet structure
 */
typedef struct mod_header {
  sos_pid_t mod_id;        //!< module ID (used for messaging).  Set NULL_PID for system selected mod_id
  uint8_t state_size;      //!< module state size
  uint8_t num_timers;      //!< Number of timers to be reserved at module load time
  uint8_t num_sub_func;	   //!< number of functions to be subscribed
  uint8_t num_prov_func;   //!< number of functions provided
  uint8_t version;         //!< version number, for users bookkeeping
  uint8_t processor_type;  //!< processor type of this module
  uint8_t platform_type;   //!< platform type of this module
  sos_code_id_t code_id;   //!< module image identifier
  uint8_t padding;
  uint8_t padding2; 
  msg_handler_t module_handler;
  func_cb_t funct[];
} mod_header_t;

/**
 * @brief Per module data structure
 */
typedef struct Module {
  //! link list to next module
  struct Module *next;
  //! module header
  mod_header_ptr header;
  //! module id
  sos_pid_t pid;
  //! message rules and kernel flag for resource management
  sos_ker_flag_t flag;
  //! per handler state
  void *handler_state;
} sos_module_t;

/** 
 * Flag used in ker_spawn_module
 */
enum {
	SOS_CREATE_THREAD    =  1,
};

/**
 * @brief module operations
 *
 * the op code is used to inform module distribution protocol the
 * operation that a user executed regarding a particular module
 */
enum {
  MODULE_OP_INSMOD        = 1,   //!< insert module (load + activate)
  MODULE_OP_RMMOD         = 2,   //!< remove module
  MODULE_OP_LOAD          = 3,   //!< load module in program space(load + not activate)
  MODULE_OP_ACTIVATE      = 4,   //!< activate a module   (no loading)
  MODULE_OP_DEACTIVATE    = 5,   //!< deactivate a module (no unloading)
};

/**
 *  @brief module operation message format
 *
 *  This message might be from network or from local node
 */
typedef struct {
  sos_pid_t mod_id;
  uint8_t version;
  /* module size, valid in MODULE_OP_INSMOD and MODULE_OP_LOAD*/
  uint16_t size;
  uint8_t op;         /* module operation type, see above */
} PACK_STRUCT
sos_module_op_t;


#endif
