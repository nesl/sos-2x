#ifndef _ERROR_TYPE_H_
#define _ERROR_TYPE_H_

#include <hardware_proc.h>

/**
 * @brief error stub for function subscriber
 */
extern void error_v(func_cb_ptr);
extern int8_t error_8(func_cb_ptr);
extern int16_t error_16(func_cb_ptr);
extern int32_t error_32(func_cb_ptr);
extern void* error_ptr(func_cb_ptr);

#endif
