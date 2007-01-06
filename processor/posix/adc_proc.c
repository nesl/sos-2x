/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*									tab:4
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
// The hardware presentation layer. See hpl.h for the C side.
// Note: there's a separate C side (hpl.h) to get access to the avr macros

// The model is that HPL is stateless. If the desired interface is as stateless
// it can be implemented here (Clock, FlashBitSPI). Otherwise you should
// create a separate component


/**
 * @author Jason Hill
 * @author David Gay
 * @author Philip Levis
 * @author Phil Buonadonna
 * @author Simon Han (simonhan@ee.ucla.edu) 
 *   Port to SOS
 * @author Ram Kumar
 *   Added ADC module API
 */
#include "hardware.h"
#include <sos_sched.h>
#include <adc_proc.h>

#ifndef SOS_DEBUG_ADC
#undef DEBUG
#define DEBUG(...)
#endif

enum
  {
	MSG_SIM_ADC = PROC_MSG_START
  };

static inline void ADC_dataReady(uint16_t data);

static uint8_t TOSH_adc_portmap[TOSH_ADC_PORTMAPSIZE];
static uint8_t adc_callback[TOSH_ADC_PORTMAPSIZE];

static inline void ADC_sampleAgain();
static void ADC_samplePort(uint8_t port);

static int8_t adc_handler(void *state, Message *e);
static mod_header_t mod_header SOS_MODULE_HEADER ={
	mod_id : ADC_PID,
	state_size : 0,
	num_sub_func : 0,
	num_prov_func : 0,
	module_handler: adc_handler,
};

static sos_module_t adc_module;

static int8_t adc_handler(void *state, Message *e){
  switch (e->type){
  case MSG_SIM_ADC:
	{
	  uint16_t data = 0xffff;
	  data &= 0x3ff;
	  ADC_dataReady(data); 
	  break;
	}
  default:
	break;
  }
  return SOS_OK;
}


static inline void init_portmap() {
  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  {
    int i;
    for (i = 0; i < TOSH_ADC_PORTMAPSIZE; i++){
      TOSH_adc_portmap[i] = i;
      adc_callback[i] = NULL_PID;
    }
    
    // Setup fixed bindings associated with ATmega128 ADC 
    TOSH_adc_portmap[TOS_ADC_BANDGAP_PORT] = TOSH_ACTUAL_BANDGAP_PORT;
    TOSH_adc_portmap[TOS_ADC_GND_PORT] = TOSH_ACTUAL_GND_PORT;
  }
  LEAVE_CRITICAL_SECTION();
}

static inline void adc_proc_init() {
  init_portmap();
}


int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcport, uint8_t driverpid){
  if (port < TOSH_ADC_PORTMAPSIZE &&
      port != TOS_ADC_BANDGAP_PORT &&
      port != TOS_ADC_GND_PORT) {
    HAS_CRITICAL_SECTION;
    ENTER_CRITICAL_SECTION();
    TOSH_adc_portmap[port] = adcport;
    adc_callback[port] = driverpid;
    LEAVE_CRITICAL_SECTION();
    return 0;
  }
  else
    return -EINVAL;
}


static void ADC_samplePort(uint8_t port) {
  HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
  //  outp((TOSH_adc_portmap[port] & 0x1F), ADMUX);
  post_short(ADC_PID, ADC_PID, MSG_SIM_ADC, TOSH_adc_portmap[port] & 0x1F, 0, 0);  
  LEAVE_CRITICAL_SECTION();
}

static inline void ADC_sampleAgain() {
  //  sbi(ADCSR, ADSC);
  post_short(ADC_PID, ADC_PID, MSG_SIM_ADC, 0, 0, 0);  
}



/*  OS component abstraction of the analog to digital converter using a 
 *  fixed reference input.  I assumes the presence of a TOS_ADC_BANDGAP_PORT
 *  to provide that referenced reading. This module was designed to 
 *  accomodate platforms that use varying/unstable ADC references. It also
 *  works around limitations where the measured variable cannot be larger than
 *  the actual ADC reference
 *
 *  The conversion result is given by the equation:
 * 
 *	   ADC = (Vport * 1024) / Vref
 * 
 *  Where Vport can be between zero and (2^6-1)*Vref (I.E. Vport CAN be larger
 *  than Vref)
 * 
 *  Note: On the ATmega128, Vref (using this module) is 1.23 Volts
 */

/*  ADC_INIT command initializes the device */
/*  ADC_GET_DATA command initiates acquiring a sensor reading. */
/*  It returns immediately.   */
/*  ADC_DATA_READY is signaled, providing data, when it becomes */
/*  available. */
/*  Access to the sensor is performed in the background by a separate */
/* TOS task. */

enum {
  IDLE = 0,
  SINGLE_CONVERSION = 1,
  CONTINUOUS_CONVERSION = 2,
};

static uint16_t ReqPort;
static uint16_t ReqVector;
static uint16_t ContReqMask;
static uint32_t RefVal;



static inline void ADC_dataReady(uint16_t data) {
  HAS_CRITICAL_SECTION;
  uint16_t doneValue = data;
  uint8_t donePort;
  uint8_t nextPort = 0xff;
  int8_t Result = -EINVAL;
  
  if (ReqPort == TOS_ADC_BANDGAP_PORT) {
    RefVal = data;
  }
  // BEGIN atomic
  ENTER_CRITICAL_SECTION();
  {
    donePort = ReqPort;
    // Check to see if this port has requested continous conversio
    if (((1<<donePort) & ContReqMask) == 0) { 
      ReqVector ^= (1<<donePort); 
    }
    
    if (ReqVector) {
      // Always ensure we rotate through the reference port 
      //ReqVector |= (1<<TOS_ADC_BANDGAP_PORT); 
      do {
	ReqPort++;
	ReqPort = (ReqPort == TOSH_ADC_PORTMAPSIZE) ? 0 : ReqPort;
      } while (((1<<ReqPort) & ReqVector) == 0);
      nextPort = ReqPort;
    }
  }
  LEAVE_CRITICAL_SECTION();
  // END atomic
  
  if (nextPort != 0xff) {
    ADC_samplePort(nextPort);   // This function is interupt-safe  
  }
  
  //dbg(DBG_ADC, "adc_tick: port %d with value %i \n", donePort, (int)data);
	if (donePort == TOS_ADC_CC_RSSI_PORT) {
		post_short(adc_callback[donePort], ADC_PID, MSG_DATA_READY, donePort, doneValue, SOS_MSG_HIGH_PRIORITY);

	} else if (donePort != TOS_ADC_BANDGAP_PORT) {
		uint32_t tmp = (uint32_t) data;
		tmp = tmp << 10;  // data * 1024
		tmp = (tmp / RefVal);  // doneValue = data * 1024/ref
		doneValue = (uint16_t) tmp;
		post_short(adc_callback[donePort], ADC_PID, MSG_DATA_READY, donePort, doneValue, SOS_MSG_HIGH_PRIORITY);
	}

	ENTER_CRITICAL_SECTION();
	{
			if ((ContReqMask & (1<<donePort)) && (Result < 0)) {
      ContReqMask ^= (1<<donePort);
    }
  }
  LEAVE_CRITICAL_SECTION();
}



static int8_t startGet(uint8_t port) {
  uint16_t PortMask, oldReqVector = 1;
  int8_t Result = 0;
  
  PortMask = (1<<port);
  
  if ((PortMask & ReqVector) != 0) {
    // Already a pending request on this port
    Result = -EBUSY;
  }
  else {
    oldReqVector = ReqVector;
    ReqVector |= PortMask;
    if (oldReqVector == 0) {
      ADC_samplePort(port);
      ReqPort = port;
    }
  }
  
  return Result;
}

/**
 * Public Functions
 */

void ADCControl_init() {
  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  {
    ReqPort = 0;
    ReqVector = ContReqMask = 0;
    RefVal = 381; // Reference value assuming 3.3 Volt power source
  }
  LEAVE_CRITICAL_SECTION();
  adc_proc_init();
  sched_register_kernel_module(&adc_module, sos_get_header_address(mod_header), NULL);
}

int8_t ker_adc_proc_getData(uint8_t port) {
  HAS_CRITICAL_SECTION;
  int8_t Result;
  DEBUG("ADC: Data Requested on Port: %d\n", port);
  if (port > TOSH_ADC_PORTMAPSIZE) {
	return -EINVAL;
  }
  
  ENTER_CRITICAL_SECTION();
  {
    Result = startGet(port);
  }
  LEAVE_CRITICAL_SECTION();
  return Result;
}

int8_t ADC_getContinuousData(uint8_t port) 
{
  HAS_CRITICAL_SECTION;
  int8_t Result = 0;
  
  if (port > TOSH_ADC_PORTMAPSIZE) {
    return -EINVAL;
  }
  ENTER_CRITICAL_SECTION();
  {
    ContReqMask |= (1<<port);
    Result = startGet(port);
    if (Result < 0) {
      ContReqMask ^= (1<<port);
    }
  }
  LEAVE_CRITICAL_SECTION();
  return Result;
}

int8_t ADCControl_manualCalibrate() {
  HAS_CRITICAL_SECTION;
  int8_t Result;
  
  ENTER_CRITICAL_SECTION();
  {
    Result = startGet(TOS_ADC_BANDGAP_PORT);
  }
  LEAVE_CRITICAL_SECTION();
  return Result;
  
}

