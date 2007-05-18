#ifndef WIRING_CONFIG_INCL_H
#define WIRING_CONFIG_INCL_H

#include <fntable.h>
#include <plat_token_capture.h>
#include <vire_malloc.h>

#define INVALID_GID				0xFF

// Shared information with elements
enum {
    DISPATCH_FID    = 0,
	SIGNAL_ERR_FID,
	UPDATE_PARAM_FID	= 250,
};

//typedef int8_t (*put_token_func_t)(func_cb_ptr cb, func_cb_ptr caller_cb, void *data, uint16_t length);
typedef int8_t (*put_token_func_t)(func_cb_ptr cb, func_cb_ptr caller_cb, token_type_t *t);
typedef int8_t (*signal_error_func_t)(func_cb_ptr cb, int8_t error);

int8_t dispatch(func_cb_ptr caller_cb, token_type_t *t);

// Shared information with backend PC
// This bit mask is sent as the 'handle' field of loader
// data structure.
enum {
	HOT_SWAP_BIT	= 0x01,	//!< Force hot swap for new configuration. Default = reinstall.
	UPDATE_DIFF 	= 0x02,	//!< The wiring update is in diff format.
	MERGE_PARAMETERS	= 0x04,	//!< Merge previous saved parameters into updated values.
	PARAM_SECTION		= 0x40,	//!< Parameter config section is present.
	WIRING_SECTION		= 0x80,	//!< Wiring config section is present.
};

enum {
	EMPTY_FIELD = 0,
	OUTPUT_RECORD,
	INPUT_RECORD,
	END_RECORD,
	END_TABLE,
	BEGIN_WIRING,
	BEGIN_PARAMETERS,
	PARAMETER_RECORD,
};

// Shared information with both elements and backend PC
enum {
	INPUT_PORT_0 = 0x00, 
	INPUT_PORT_1,
	INPUT_PORT_2,
	INPUT_PORT_3,
	INPUT_PORT_4,
	INPUT_PORT_5,
	INPUT_PORT_6,
	INPUT_PORT_7,
	INPUT_PORT_8,
	INPUT_PORT_9,
	INPUT_PORT_10,
	INPUT_PORT_11,
	INPUT_PORT_12,
	INPUT_PORT_13,
	INPUT_PORT_14,
	INPUT_PORT_15,
	INPUT_PORT_16,
	INPUT_PORT_17,
	INPUT_PORT_18,
	INPUT_PORT_19,
	INPUT_PORT_20,
	INPUT_PORT_21,
	INPUT_PORT_22,
	INPUT_PORT_23,
	INPUT_PORT_24,
	INPUT_PORT_25,
	INPUT_PORT_26,
	INPUT_PORT_27,
	INPUT_PORT_28,
	INPUT_PORT_29,
	INPUT_PORT_30,
	INPUT_PORT_31, 
};

#define INPUT_PORT_OFFSET_FID	INPUT_PORT_0

enum {
	OUTPUT_PORT_0 = 0x00, // 0
	OUTPUT_PORT_1,
	OUTPUT_PORT_2,
	OUTPUT_PORT_3,
	OUTPUT_PORT_4,
	OUTPUT_PORT_5,
	OUTPUT_PORT_6,
	OUTPUT_PORT_7,
	OUTPUT_PORT_8,
	OUTPUT_PORT_9,
	OUTPUT_PORT_10,
	OUTPUT_PORT_11,
	OUTPUT_PORT_12,
	OUTPUT_PORT_13,
	OUTPUT_PORT_14,
	OUTPUT_PORT_15,
	OUTPUT_PORT_16,
	OUTPUT_PORT_17,
	OUTPUT_PORT_18,
	OUTPUT_PORT_19,
	OUTPUT_PORT_20,
	OUTPUT_PORT_21,
	OUTPUT_PORT_22,
	OUTPUT_PORT_23,
	OUTPUT_PORT_24,
	OUTPUT_PORT_25,
	OUTPUT_PORT_26,
	OUTPUT_PORT_27,
	OUTPUT_PORT_28,
	OUTPUT_PORT_29,
	OUTPUT_PORT_30,
	OUTPUT_PORT_31,     // 31
};



#endif
