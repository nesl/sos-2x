/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _FNTABLE_TYPES_H_
#define _FNTABLE_TYPES_H_

/**
 * @brief function control block
 *
 * For provider: ptr points to real implementation
 *               proto stores function prototype
 *               pid is module ID
 *               fid is function ID
 *
 * For static user: ptr is default error handler
 *                  proto stores function prototype
 *                  pid is module ID of provider
 *                  fid is function ID of provider
 *
 * For dynamic user: ptr is default error handler
 *                  proto stores function prototype
 *                  pid is RUNTIME_PID
 *                  fid is RUNTIME_FID
 *
 * @note When modifying this structure, please keep it even size.
 * The purpose is to keep func_cb align in 16 bits.
 */
#include <hardware_proc.h>
#define RUNTIME_FID    255
#define RUNTIME_PID    NULL_PID  // Please make sure this value is 255

// NOTE: PLEASE MAKE SURE THAT THIS STRUCTURE
// IS WORD ALIGNED.
typedef struct func_cb {
	void *ptr;        //! function pointer                    
	uint8_t proto[4]; //! function prototype                  
	uint8_t pid;      //! function PID                                    
	uint8_t fid;      //! function ID                         
} func_cb_t;

/**
 * \ingroup coreApi
 * \defgroup dymfunc Dynamic Functions
 * Function for invoking dyanmic functions
 * \note Do not use SOS_CALL within control statement such as while, if, switch
 * if the return value of SOS_CALL is needed, save it in a temporary variable 
 * and then use the temporary variable in the control statement. 
 * AVR architecture does not have this limitation because SOS_CALL is 
 * implemented in assembly.  
 * @{
 */
#ifdef SYS_JUMP_TBL_START

#if defined(SOS_SFI) && !defined(_MODULE_)
#define SOS_CALL(fnptrptr,type, args...)  \
(((type)(SYS_JUMP_TBL_START + SYS_JUMP_TBL_SIZE*20))((fnptrptr), ##args))
#else
#define SOS_CALL(fnptrptr,type, args...)  \
(((type)(SYS_JUMP_TBL_START))((fnptrptr), ##args))
#endif //SOS_SFI && !_MODULE_

#else
#define SOS_CALL(fnptrptr,type, args...)  \
((type)(ker_sys_enter_func(fnptrptr)))((fnptrptr), ##args); ker_sys_leave_func()
#endif
/* @} */
typedef void (*dummy_func)(void);
typedef int8_t (*fn_ptr_t)(void);

dummy_func ker_sys_enter_func( func_cb_ptr p );
void ker_sys_leave_func( void );

#endif

