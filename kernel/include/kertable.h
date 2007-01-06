/**
 * @brief Kertable.h defines kernel jump table in a portable fashion
 * The main goal of this file is to define kernel jump table at exactly 
 * one place
 */
#ifndef _KERTABLE_H_
#define _KERTABLE_H_
#include <sos_info.h>
#include <sos_timer.h>
#ifndef _MODULE_
#include <sys_module_in.h>
#include <module.h>
/*
 * This is the kernel jump table.
 * The wrapper for using jump table is implemented in module.h and sys_module.h
 * 
 */

#define SOS_KER_TABLE(...) {						\
    /*  0 */ (void*)ker_sys_malloc,						\
      /*  1 */ (void*)ker_sys_realloc,					\
      /*  2 */ (void*)ker_sys_free,						\
      /*  3 */ (void*)ker_sys_msg_take_data,				\
      /*  4 */ (void*)ker_sys_timer_start,					\
      /*  5 */ (void*)ker_sys_timer_restart,				\
      /*  6 */ (void*)ker_sys_timer_stop,					\
      /*  7 */ (void*)ker_sys_post,						\
      /*  8 */ (void*)ker_sys_post_link,					\
      /*  9 */ (void*)ker_sys_post_value,                    		\
      /* 10 */ (void*)ker_hw_type,					\
      /* 11 */ (void*)ker_id,						\
      /* 12 */ (void*)ker_rand,						\
      /* 13 */ (void*)ker_systime32,					\
      /* 14 */ (void*)ker_panic,					\
      /* 15 */ (void*)ker_panic,					\
      /* 16 */ (void*)ker_panic,					\
      /* 17 */ (void*)ker_panic,					\
      /* 18 */ (void*)ker_msg_take_data,				\
      /* 19 */ (void*)ker_msg_change_rules,				\
      /* 20 */ (void*)ker_timer_init,					\
      /* 21 */ (void*)ker_timer_start,					\
      /* 22 */ (void*)ker_timer_restart,				\
      /* 23 */ (void*)ker_timer_stop,					\
      /* 24 */ (void*)ker_timer_release,				\
      /* 25 */ (void*)post_link,					\
      /* 26 */ (void*)post,						\
      /* 27 */ (void*)post_short,					\
      /* 28 */ (void*)post_long,					\
      /* 29 */ (void*)post_longer,					\
      /* 30 */ (void*)ker_loc,						\
      /* 31 */ (void*)ker_gps,						\
      /* 32 */ (void*)ker_loc_r2,					\
      /* 33 */ (void*)ker_systime16L,					\
      /* 34 */ (void*)ker_systime16H,					\
      /* 35 */ (void*)ker_register_module,				\
      /* 36 */ (void*)ker_deregister_module,				\
      /* 37 */ (void*)ker_get_module,					\
      /* 38 */ (void*)ker_register_monitor,				\
      /* 39 */ (void*)ker_deregister_monitor,				\
      /* 40 */ (void*)ker_fntable_subscribe,				\
      /* 41 */ (void*)ker_sensor_register,				\
      /* 42 */ (void*)ker_sensor_deregister,				\
      /* 43 */ (void*)ker_sensor_get_data,				\
      /* 44 */ (void*)ker_sensor_data_ready,				\
      /* 45 */ (void*)ker_sensor_enable,				\
      /* 46 */ (void*)ker_sensor_disable,				\
      /* 47 */ (void*)ker_sensor_control,				\
      /* 48 */ (void*)ker_set_current_pid,				\
      /* 49 */ (void*)ker_get_current_pid,				\
      /* 50 */ (void*)ker_get_module_state,				\
      /* 51 */ (void*)ker_spawn_module,					\
      /* 52 */ (void*)ker_codemem_get_header_from_code_id,		\
      /* 53 */ (void*)ker_codemem_get_header_address,			\
      /* 54 */ (void*)ker_codemem_alloc,				\
      /* 55 */ (void*)ker_codemem_write,				\
      /* 56 */ (void*)ker_codemem_read,					\
      /* 57 */ (void*)ker_codemem_free,					\
      /* 58 */ (void*)ker_codemem_flush,				\
      /* 59 */ (void*)ker_get_func_ptr,					\
      /* 60 */ (void*)ker_panic,					\
      /* 61 */ (void*)ker_mod_panic,					\
      __VA_ARGS__							\
      } 

#endif
#define SYS_KERTABLE_LEN 64
#define SYS_KERTABLE_END 61

#define CONCAT_TABLES(_table_,...)		\
  _table_					\
  __VA_ARGS__

#endif

