#ifndef _KERTABLE_PLAT_H_
#define _KERTABLE_PLAT_H_

#include <kertable_proc.h>

#define PLAT_KER_TABLE					\
        /*  1 */ (void*)ker_spi_reserve_bus,		\
        /*  2 */ (void*)ker_spi_release_bus,		\
        /*  3 */ (void*)ker_spi_send_data,		\
        /*  4 */ (void*)ker_spi_read_data,		\
        /*  5 */ (void*)ker_vref,			\
        /*  6 */ (void*)ker_preamp,			\
        /*  7 */ (void*)ker_ads8341_adc_bindPort,	\
        /*  8 */ (void*)ker_ads8341_adc_getData,	\
        /*  9 */ (void*)ker_ads8341_adc_getPerodicData,	\
        /* 10 */ (void*)ker_ads8341_adc_getDataDMA,	\
        /* 11 */ (void*)ker_ads8341_adc_stopPerodicData, \
        /* 12 */ (void*)ker_ltc6915_amp_setGain,	\
        /* 13 */ (void*)ker_vreg,			\
        /* 14 */ (void*)ker_switches,			\
        /* 15 */ (void*)ker_adg715_set_mux,		\
        /* 16 */ (void*)ker_adg715_read_mux,		\
        /* 17 */ (void*)ker_spi_register_read_cb,	\
        /* 18 */ (void*)ker_spi_unregister_read_cb,	\
        /* 19 */ (void*)ker_spi_register_send_cb,	\
        /* 20 */ (void*)ker_spi_unregister_send_cb,	\

#define PLAT_KERTABLE_LEN 20
#define PLAT_KERTABLE_END (PROC_KERTABLE_END+PLAT_KERTABLE_LEN)

#endif
