/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 *
 */

#ifndef _ADC_PROC_H
#define _ADC_PROC_H
#if defined(_SOS_KERNEL_)
#include <sos.h>
#else
#include <module.h>
#endif
#include <adc_cb.h>

#include <adc_proc_hw.h>

#include "adc_proc.h"


// 8 ports for the user 2 for the system
#define ADC_PROC_PORTMAPSIZE 8
#define ADC_PROC_EXTENDED_PORTMAPSIZE 10

enum {
	ADC_PROC_PORT0=0,
	ADC_PROC_PORT1,
	ADC_PROC_PORT2,
	ADC_PROC_PORT3,
	ADC_PROC_PORT4,
	ADC_PROC_PORT5,
	ADC_PROC_PORT6,
	ADC_PROC_PORT7,
	// system calibration ports
	ADC_PROC_SYS_PORT8,
	ADC_PROC_SYS_PORT9,
};

#ifndef _MODULE_
/** 
 * @brief init function for ADC
 */
extern int8_t adc_proc_init();

/**
 * @brief bind port with callback 
 * The port will be used in ker_adc_proc_getData and ker_adc_proc_getContinuousData
 */
extern int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid);
extern int8_t ker_adc_proc_unbindPort(uint8_t port, sos_pid_t calling_id);


/**
 * @brief Get ADC data
 * @param uint8_t port Logical ADC port
 * @return int8_t SOS_OK if valid port and available; -EBUSY if port is busy; -EINVAL for invalid port
 */
extern int8_t ker_adc_proc_getData(uint8_t port, uint8_t flags);


/**
 * @brief Get continuous ADC data
 * @param uint8_t port Logical ADC port
 * @return int8_t SOS_OK if valid port and available; -EBUSY if port is busy; -EINVAL for invalid port
 */
extern int8_t ker_adc_proc_getPerodicData(uint8_t port, uint8_t prescaler, uint16_t count);
extern int8_t ker_adc_proc_stopPerodicData(uint8_t port);

#endif // _MODULE_

#endif // _ADC_PROC_H 

