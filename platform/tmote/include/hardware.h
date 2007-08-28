/**
 * @brief    TMote Sky hardware definitions
 * @author   Ram Kumar {ram@ee.ucla.edu}
 * $Id: hardware.h,v 1.11 2006/06/22 00:48:01 hubert Exp $
 */
//----------------------------------------------------------------
// TMOTE SKY HARDWARE PLATFORM
//----------------------------------------------------------------
#ifndef _HARDWARE_H
#define _HARDWARE_H

#include <stdio.h>
// system headers
#include <hardware_types.h> // Include the processor specific types
#include <sos_types.h>      // Commonly used data-types in SOS
#include <pin_map.h>

#include "timer.h"          // Include the timer specific definitions
#include "systime.h"        // Prototypes of systime interface for kertable
#include "uart.h"
#include "led.h"
#include <clock_hal.h>
#include <timerb_hal.h>
#include <spi_hal.h>
//#include <cc2420_hal.h>
#include <vhal.h>
#include <vmac.h>
#include <adc_proc.h>
#include <adc_driver_kernel.h>
#include <interrupt_ctrl_kernel.h>

/**
 * @brief initialize hardware
 */
extern void hardware_init(void);

// I/O setup for the Telos platform
#ifndef NO_SOS_RADIO
#define SOS_RADIO_CHANNEL
#define SOS_RADIO_LINK_DISPATCH(m) radio_msg_alloc(m)
#endif

#ifndef NO_SOS_UART
#define SOS_UART_CHANNEL
#define SOS_UART_LINK_DISPATCH(m) uart_msg_alloc(m)
#endif


static inline void TOSH_wait(void) {
  nop(); nop();
}

static inline void TOSH_wait_250ns(void) {
  // 4 MHz clock == 1 cycle per 250 ns
  nop();
}

static inline void TOSH_uwait(uint16_t u) {
  uint16_t i;
  if (u < 500) {
    for (i=2; i < u; i++) { 
      asm volatile("nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   ::);
    }
  } else {
    for (i=0; i < u; i++) {
      asm volatile("nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   "nop\n\t"
		   ::);
    }
  }
}

#endif

