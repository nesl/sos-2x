#ifndef _KERTABLE_PROC_H_
#define _KERTABLE_PROC_H_

#include <kertable.h> // for SYS_KERTABLE_END

#define PROC_KER_TABLE                                       \
	/* 1 */ (void*)ker_radio_ack_enable,                   \
	/* 2 */ (void*)ker_radio_ack_disable,                  \
	/* 3 */ (void*)ker_adc_proc_bindPort,                       \
	/* 4 */ (void*)ker_adc_proc_getData,                        \
	/* 5 */ (void*)ker_adc_proc_getContinuousData,              \
	/* 6 */ (void*)ker_adc_proc_getCalData,                     \
	/* 7 */ (void*)ker_adc_proc_getCalContinuousData,           \
	/* 8 */ NULL

#define PROC_KERTABLE_LEN 8

#define PROC_KERTABLE_END (SYS_KERTABLE_END+PROC_KERTABLE_LEN)

#endif

