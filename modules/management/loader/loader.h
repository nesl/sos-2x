
#ifndef _SOS_LOADER_H
#define _SOS_LOADER_H

#include <sos_module_fetcher.h>
/**                                                           
 * Messages                                                   
 */                                                           
enum {                                                        
	MSG_VERSION_ADV            = ( MOD_MSG_START + 0 ),
	MSG_VERSION_DATA           = ( MOD_MSG_START + 1 ),               
	MSG_LOADER_LS_ON_NODE      = ( MOD_MSG_START + 2 ),
	MSG_LOADER_LS_REPLY        = ( MOD_MSG_START + 3 ),

	//! used by user
	MSG_LOADER_INSMOD          = ( MOD_CMD_START + 4 ),               
	MSG_LOADER_LSMOD           = ( MOD_CMD_START + 5 ),
	MSG_LOADER_RMMOD           = ( MOD_CMD_START + 6 ),
	MSG_LOADER_LDMOD           = ( MOD_CMD_START + 7 ),
	MSG_LOADER_LDDATA          = ( MOD_CMD_START + 8 ),
	MSG_LOADER_RMDATA          = ( MOD_CMD_START + 9 ),
	MSG_LOADER_LSDATA          = ( MOD_CMD_START + 10 ),
	MSG_LOADER_DATA_AVAILABLE  = ( MOD_MSG_START + 11 ),
};     

typedef uint16_t version_t;

#define version_hton( v )  (ehtons((v)))
#define version_ntoh( v )  (entohs((v)))

enum {
	NUM_LOADER_PARAMS_ENTRIES     =    8,       
	NUM_LOADER_MODULE_ENTRIES     =    8,
	NUM_LOADER_KERNEL_ENTRIES     =    1,
	LOADER_PARAM_SIZE             =  256L,
	LOADER_SIZE_MULTIPLIER        =   32L,   
};

/**                                                           
 * Message Content ( MSG_VERSION_ADV )                        
 */                                                           
typedef struct {                                              
	version_t version;                                        
} msg_version_adv_t;                                          

/**                                                           
 * Message Content ( MSG_VERSION_DATA )                       
 * 
 * @note size is always multiple of LOADER_SIZE_MULTIPLIER
 * Conversion: 
 *   stored_size = 
 *      (real_size + (LOADER_SIZE_MULTIPLIER - 1)) / LOADER_SIZE_MULTIPLIER
 *   load_size = stored_size * LOADER_SIZE_MULTIPLIER
 *
 */
typedef struct {                                              
	version_t version;                                        
	uint8_t  pam_ver  [ NUM_LOADER_PARAMS_ENTRIES ];
	uint8_t  pam_size [ NUM_LOADER_PARAMS_ENTRIES ];  
	uint8_t  mod_ver  [ NUM_LOADER_MODULE_ENTRIES ];
	uint8_t  mod_size [ NUM_LOADER_MODULE_ENTRIES ];  
	uint8_t  ker_ver  [ NUM_LOADER_KERNEL_ENTRIES ];
	uint16_t ker_size [ NUM_LOADER_KERNEL_ENTRIES ];
} PACK_STRUCT
msg_version_data_t;

typedef struct {
	uint8_t pam_dst_pid[ NUM_LOADER_PARAMS_ENTRIES ];
	uint8_t pam_dst_subpid[ NUM_LOADER_PARAMS_ENTRIES ];
	sos_code_id_t code_id [ NUM_LOADER_MODULE_ENTRIES ];
} PACK_STRUCT
msg_ls_reply_t;

typedef struct {
  fetcher_cam_t fetcher;
  uint8_t version;
  uint8_t code_size;
} PACK_STRUCT
loader_cam_t;
#endif

