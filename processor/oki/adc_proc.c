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


//-----------------------------------------------------------------------------
// INCLUDES
#include <sos.h>
#include "hardware.h"
#include <sos_sched.h>
#include <sos_timer.h>

#include <adc_proc.h>

#define ADC_MODULE_PID		DFLT_APP_ID0 + 10
//-----------------------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
static inline void HPLADC_init();
static inline void init_portmap();
static void ADC_setSamplingRate(uint8_t rate);
static inline void ADC_dataReady(uint16_t data);
//static inline void ADC_sampleAgain();
static void ADC_samplePort(uint8_t port);
static void ADC_interruptHandler();

//-----------------------------------------------------------------------------
// LOCAL VARIABLES
static uint8_t TOSH_adc_portmap[TOSH_ADC_PORTMAPSIZE];
static uint8_t adc_callback[TOSH_ADC_PORTMAPSIZE];
static uint16_t ReqPort;
static uint16_t ReqVector;

//-----------------------------------------------------------------------------
// ADC STATE
enum
  {
	IDLE = 0,
	SINGLE_CONVERSION = 1,
	CONTINUOUS_CONVERSION = 2,
  };

/*
typedef struct {
	uint8_t pid;
} adc_module_state_t;

static int8_t adc_module_manager (void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER ={
mod_id : ADC_MODULE_PID,
num_timers : 0,	//TOSH_ADC_PORTMAPSIZE,
state_size : 0,
num_sub_func : 0,
num_prov_func : 0,
module_handler: adc_module_manager,
};
static sos_module_t adc_module;
static adc_module_state_t adc_state;
*/

//static sos_timer_t adc_timer[TOSH_ADC_PORTMAPSIZE];

//-----------------------------------------------------------------------------
// INITIALIZE ADC
void adc_proc_init()
{
	//sched_register_kernel_module(&adc_module, sos_get_header_address(mod_header), &adc_state);
  	HAS_CRITICAL_SECTION;
  	ENTER_CRITICAL_SECTION();
  	{
  	  	ReqPort = 0;
    	ReqVector = 0;
  	}
  	LEAVE_CRITICAL_SECTION();
  	HPLADC_init();
}

//-----------------------------------------------------------------------------
// INITIALIZE HPL ADC
static inline void HPLADC_init()
{
  DEBUG("HPLADC init\n\r");
  HAS_CRITICAL_SECTION;
  init_portmap();
  // Enable ADC Interupts,
  // Set Prescaler division factor to 64
  ENTER_CRITICAL_SECTION();
  {
	IRQ_HANDLER_TABLE[INT_AD] = ADC_interruptHandler;
	set_wbit(ILC1, ILC1_ILR11 & ILC1_INT_LV1);
	put_wvalue(ADINT, ADINT_ADSTIE);	// enable select interrupt
    ADC_setSamplingRate(ADCON2_CLK2);
  }
  LEAVE_CRITICAL_SECTION();
}

//-----------------------------------------------------------------------------
// INITIALIZE PORTMAP
static inline void init_portmap()
{
  HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
  {
    int i;
    for (i = 0; i < TOSH_ADC_PORTMAPSIZE; i++){
      TOSH_adc_portmap[i] = i;
      adc_callback[i] = NULL_PID;
    }
  }
  LEAVE_CRITICAL_SECTION();
}

//-----------------------------------------------------------------------------
// ADC SET SAMPLING RATE
static void ADC_setSamplingRate(uint8_t rate) {
	if (rate > ADCON2_CLK8 || rate < ADCON2_CLK2)
		return;
	put_wvalue(ADCON1, rate);
}


//-----------------------------------------------------------------------------
// SYSTEM CALL API
// BIND A PORT ABSTRACTION TO ACTUAL ADC PORT AND ASSOCIATE WITH A MODULE		// consider channel/port range and remove uint8_t adcport
//int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t type, uint32_t period, sos_pid_t driverpid){
int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcport, sos_pid_t driverpid){
  if (port < TOSH_ADC_PORTMAPSIZE) {
	DEBUG("ADC bind port %u to %u\n\r", port, driverpid);
    HAS_CRITICAL_SECTION;
    ENTER_CRITICAL_SECTION();
    TOSH_adc_portmap[port] = adcport;
    adc_callback[port] = driverpid;
	//sos_timer_init(&(adc_timer[port]), ADC_MODULE_PID, port, type);
	//ker_timer_start(&(adc_timer[port]), period);
    LEAVE_CRITICAL_SECTION();
    return 0;
  }
  else
    return -EINVAL;
}

//-----------------------------------------------------------------------------
// SAMPLE HW ADC PORT
static void ADC_samplePort(uint8_t port) {
  DEBUG("ADC sample port %u\n\r", port);
  HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
  put_wvalue(ADCON1, (TOSH_adc_portmap[port] & ADCON1_ADSTM));
  LEAVE_CRITICAL_SECTION();
  put_wvalue(ADCON1, (get_wvalue(ADCON1) | ADCON1_STS));
}

//-----------------------------------------------------------------------------
// ADC HW INTERRUPT
static void ADC_interruptHandler() {
  	HAS_CRITICAL_SECTION;
  	ENTER_CRITICAL_SECTION();
  	uint16_t data;
#ifdef SOS_IDLE_MEASURE
	sbi(PORTC, 1);
#endif
  	switch (ReqPort) {
	  	case 0: {
		  	data = get_wvalue(ADR0);
  			data &= ADR0_DT0;
  			break;
		}
	  	case 1: {
		  	data = get_wvalue(ADR1);
  			data &= ADR1_DT1;
  			break;
		}
	  	case 2: {
		  	data = get_wvalue(ADR2);
  			data &= ADR2_DT2;
  			break;
		}
	  	case 3: {
		  	data = get_wvalue(ADR3);
  			data &= ADR3_DT3;
  			break;
		}
		default:
			data = 0;
	}
  	put_wvalue(ADINT, (ADINT_INTST | ADINT_ADSTIE)); // clear and enable interrupt
  	LEAVE_CRITICAL_SECTION();
  	ADC_dataReady(data);
}

//-----------------------------------------------------------------------------
// ADC HW INTERRUPT SERVICE ROUTINE
static inline void ADC_dataReady(uint16_t data) {
  HAS_CRITICAL_SECTION;
  uint16_t doneValue = data;
  uint8_t donePort;
  uint8_t nextPort = 0xff;
  int8_t Result = -EINVAL;

  ENTER_CRITICAL_SECTION();
  DEBUG("ADC data ready %u\n\r", data);
  {
    donePort = ReqPort;
    ReqVector ^= (1<<donePort);
	if (ReqVector) {
	  do {
		ReqPort++;
		ReqPort = (ReqPort == TOSH_ADC_PORTMAPSIZE) ? 0 : ReqPort;
	  } while (((1<<ReqPort) & ReqVector) == 0);
	  nextPort = ReqPort;
	}
  }
  LEAVE_CRITICAL_SECTION();
  if (nextPort != 0xff) {
	ADC_samplePort(nextPort);   // This function is interupt-safe
  }
  Result = post_short(adc_callback[donePort], ADC_PID, MSG_DATA_READY, donePort, doneValue, SOS_MSG_HIGH_PRIORITY);
}

//-----------------------------------------------------------------------------
// STARTGET - INITIATE THE ADC PROCESS			// Should we put CRITICAL SECTION inside here?
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

//-----------------------------------------------------------------------------
// SYSTEM CALL API
// GET DATA FROM SPECIFIED ADC PORT
int8_t ker_adc_proc_getData(uint8_t port) {
  HAS_CRITICAL_SECTION;
  int8_t Result;
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


/**
 * @brief Get continuous ADC data
 * @param uint8_t port Logical ADC port
 * @return int8_t SOS_OK if valid port and available; -EBUSY if port is busy; -EINVAL for invalid port
 */
int8_t ker_adc_proc_getCalData(uint8_t port)
{
	return 0;
}

/**
 * @brief Get continuous ADC data
 * @param uint8_t port Logical ADC port
 * @return int8_t SOS_OK if valid port and available; -EBUSY if port is busy; -EINVAL for invalid port
 */
int8_t ker_adc_proc_getContinuousData(uint8_t port)
{
	return 0;
}

/**
 * @brief Get Continuous Calibrated ADC data
 * @param uint8_t port Logical ADC port
 * @return int8_t SOS_OK if valid port and available; -EBUSY if port is busy; -EINVAL for invalid port
 */
int8_t ker_adc_proc_getCalContinuousData(uint8_t port)
{
	return 0;
}

//-----------------------------------------------------------------------------
// SYSTEM CALL API
// GET DATA FROM SPECIFIED ADC PORT

int8_t ker_adc_proc_startData(uint8_t port, uint8_t type, uint32_t period) {
  if (port > TOSH_ADC_PORTMAPSIZE) {
	return -EINVAL;
  }
//  sos_timer_init(&(adc_timer[port]), ADC_MODULE_PID, port, type);
//  ker_timer_restart(&(adc_timer[port]), period);
	ker_timer_init(ADC_MODULE_PID,port,type);
	ker_timer_restart(ADC_MODULE_PID,port,period);
  return 0;
}

//-----------------------------------------------------------------------------
// SYSTEM CALL API
// GET DATA FROM SPECIFIED ADC PORT
int8_t ker_adc_proc_stopData(uint8_t port) {
  if (port > TOSH_ADC_PORTMAPSIZE) {
	return -EINVAL;
  }
//  ker_timer_stop(&(adc_timer[port]));
	ker_timer_stop(ADC_MODULE_PID,port);
  return 0;
}

//-----------------------------------------------------------------------------
// ADC Module Manager
//
/*
static int8_t adc_module_manager (void *state, Message *msg) {


	adc_module_state_t *s = (adc_module_state_t*) state;

	//led_yellow_toggle();
	switch (msg->type) {

		case MSG_INIT:
		{
			s->pid = msg->did;
  			HAS_CRITICAL_SECTION;
  			ENTER_CRITICAL_SECTION();
  			{
  			  	ReqPort = 0;
    			ReqVector = 0;
  			}
  			LEAVE_CRITICAL_SECTION();
  			HPLADC_init();
  			//ker_register_timer(s->pid,0,TIMER_REPEAT,	// start it when we need it
			return SOS_OK;
		}
		case MSG_FINAL:
		{
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			MsgParam *p = (MsgParam*)(msg->data);
			//led_green_toggle();
			//for (s->chan=0; s->chan<4; ++(s->chan)) {
			//	result = ker_adc_proc_getData(s->chan);
		  	//	//NDEBUG("adc get data %u %u", s->chan, result);
			//}
		  	//if (++(s->chan) > 3)
		  	//	s->chan = 0;

			//			uint8_t result = ker_adc_proc_getData(p->byte);	// driverpid
			ker_adc_proc_getData(p->byte);	// driverpid

		  	//NDEBUG("adc get data %u %u", s->chan, result);
			return SOS_OK;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}
*/


