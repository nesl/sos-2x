
#ifndef _KERTABLE_OKI_H_
#define _KERTABLE_OKI_H_

#define KER_TABLE_OKI                                       \
	/* 64 */ (void*)ker_radio_ack_enable,                   \
	/* 65 */ (void*)ker_radio_ack_disable,                  \
	/* 66 */ (void*)ker_adc_proc_bindPort,                       \
	/* 67 */ (void*)ker_adc_proc_getData,                        \
	/* 68 */ (void*)ker_adc_proc_getContinuousData,              \
	/* 69 */ (void*)ker_adc_proc_getCalData,                     \
	/* 70 */ (void*)ker_adc_proc_getCalContinuousData,           \
	/* 71 */ (void*)ker_i2c_slave_start,                \
	/* 72 */ (void*)ker_i2c_send_data,                      \
	/* 73 */ (void*)ker_i2c_read_data,                      \
	/* 74 */ (void*)ker_i2c_register_slave,                 \
	/* 75 */ (void*)ker_i2c_release_slave,                  \
	/* 128 */ NULL

#endif

