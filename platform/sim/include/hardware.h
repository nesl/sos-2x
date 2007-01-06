/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
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
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
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
 * @brief    hardware related routines and definitions
 * @author   Simon Han (simonhan@ee.ucla.edu)
 *
 */


#ifndef _SOS_HARDWAR_H
#define _SOS_HARDWAR_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware_types.h"
#include <sos_types.h>
#include <sos.h>
#include <sys/time.h>
#include <errno.h>
#include "systime.h"
#include "radio.h"
#include "led.h"
#include "timer.h"
#include "uart_system.h"
#include "i2c_system.h"
#include "adc_proc.h"
#include "exflash.h"
#include <sos_uart.h>

#define SOS_HAS_EXFLASH

#ifndef NO_SOS_RADIO
#define SOS_RADIO_CHANNEL
#define SOS_RADIO_LINK_DISPATCH(m) radio_msg_alloc(m)
#endif

#ifndef NO_SOS_UART
#define SOS_UART_CHANNEL
#define SOS_UART_LINK_DISPATCH(m) uart_msg_alloc(m)
#endif

#endif

