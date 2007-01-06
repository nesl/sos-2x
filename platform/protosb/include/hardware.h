/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2: */
/**
 * @brief    hardware related routines and definitions
 * @author   Naim Busek (ndbusek@lecs.cs.ucla.edu)
 *
 */


#ifndef _SOS_HARDWARE_H
#define _SOS_HARDWARE_H

// system headers
#include <hardware_types.h>
#include <hardware_proc.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <sos_types.h>
#include <pin_map.h>  // must be included before driver includes
#include <plat_msg_types.h>

// processor headers
#include "systime.h"
#include "timer.h"
#include "adc_proc.h"
#include "spi.h"
#include "spi_system.h"
#include "i2c_system.h"
#include "uart_system.h"
#ifdef SOS_SFI
#include <memmap.h>
#endif

// platform headers
#include <sos_uart.h>
//#include "drivers_plat.h"
#include "dbg.h"
#include "ads8341_adc_hal.h"
#include "ads8341_adc.h"
#include "preamp.h"
#include "switches.h"
#include "ltc6915.h"
#include "ltc6915_amp_hal.h"
#include "ltc6915_amp.h"
#include "vref.h"
#include "vreg.h"
#include "adg715_mux_hal.h"
#include "adg715_mux.h"

/**
 * @brief initialize hardware
 */
extern void hardware_init(void);


#ifndef NO_SOS_UART
#define SOS_UART_CHANNEL
#define SOS_UART_LINK_DISPATCH(m) uart_msg_alloc(m)
#endif

#ifndef NO_I2C_UART
#define SOS_I2C_CHANNEL
#define SOS_I2C_LINK_DISPATCH(m) i2c_msg_alloc(m)
#endif

#endif

