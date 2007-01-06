/*
 * Copyright (c) 2006 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * @brief    Declare the hardware on the iMote2
 * @author   Andrew Barton-Sweeney (abs@cs.yale.edu)
 */

#ifndef _HARDWARE_H
#define _HARDWARE_H

/**
 *  System headers
 */
#include <hardware_types.h>
#include <sos_types.h>
#include <pin_map.h>
#include <plat_msg_types.h>

/**
 *  Driver headers
 */
#include "uart_system.h"
#include "i2c_system.h"

/**
 *  Processor headers
 */
#include "hardware_proc.h"
#include "systime.h"
#include "timer.h"
#include "uart.h"
#include "adc_proc.h"

/**
 *  Platform headers
 */
#include "led.h"
//#include "radio.h"
#include "irq.h"
#include "pmic.h"
#include "vmac.h"
#include "gpio.h"

/**
 *  Configure the communication channels
 */
#ifndef NO_SOS_RADIO
#define SOS_RADIO_CHANNEL
#define SOS_RADIO_LINK_DISPATCH(m) radio_msg_alloc(m)
#endif

#ifndef NO_SOS_UART
#define SOS_UART_CHANNEL
#define SOS_UART_LINK_DISPATCH(m) uart_msg_alloc(m)
#endif

#ifndef NO_SOS_I2C
#define SOS_I2C_CHANNEL
#define SOS_I2C_LINK_DISPATCH(m) i2c_msg_alloc(m)
#endif

/**
 *  Initialize the hardware
 */
extern void hardware_init(void);

#endif // _HARDWARE_H
