#ifndef _KERTABLE_PROC_H_
#define _KERTABLE_PROC_H_

#include <kertable.h> // for SYS_KERTABLE_END

// NOTE - If you add a function to the PROC_KERTABLE, make sure to change
// PROC_KERTABLE_LEN
#define PROC_KER_TABLE                                          \
    /*  1 */ (void*)ker_adc_proc_bindPort,			\
    /*  2 */ (void*)ker_adc_proc_unbindPort,			\
    /*  3 */ (void*)ker_adc_proc_getData,			\
    /*  4 */ (void*)ker_adc_proc_getPerodicData,		\
    /*  5 */ (void*)ker_adc_proc_stopPerodicData,		\
    /*  6 */ (void*)ker_i2c_reserve_bus,			\
    /*  7 */ (void*)ker_i2c_release_bus,			\
    /*  8 */ (void*)ker_i2c_send_data,				\
    /*  9 */ (void*)ker_i2c_read_data,				\
    /* 10 */ (void*)ker_uart_reserve_bus,			\
    /* 11 */ (void*)ker_uart_release_bus,			\
    /* 12 */ (void*)ker_uart_send_data,				\
    /* 13 */ (void*)ker_uart_read_data,				\
    
// NOTE - Make sure to change the length if you add new functions to the
// PROC_KERTABLE
#define PROC_KERTABLE_LEN 13

#define PROC_KERTABLE_END (SYS_KERTABLE_END+PROC_KERTABLE_LEN)

#endif

