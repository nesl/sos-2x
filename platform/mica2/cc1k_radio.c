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

/**
 * \file cc1k_radio.c
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \author Simon Han {simonhan@ee.ucla.edu}
 * \brief This is the layer 2 functionality of the mica2 radio for the SOS
 * This driver has been ported from the TinyOS CC1000 Radio Stack
 */

//-----------------------------------------------------------------------------
// INCLUDES
#include <CC1000Const.h>
#include <random.h>
#include <hardware.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <malloc.h>
#include <net_stack.h>
#include <timestamp.h>
#include <sos_info.h>
#include <message.h>
#include <string.h> // for memcpy
#include "cc1k_lpl.h"
#include "crc.h"

#include <led_dbg.h>


#define MICA2_CC_RSSI_FLAGS	(0)

//-----------------------------------------------------------------------------
// STACK TIMERS
#define WAKEUP_TIMER_TID    0 //!< Wakeup Timer TID
#define SQUELCH_TIMER_TID   1 //!< Squelch Timer TID


static int8_t cc1k_radio_handler(void *state, Message *e);

// function registered with kernel sensor component
static int8_t radio_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t radio_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);


static mod_header_t mod_header SOS_MODULE_HEADER ={    
mod_id : RADIO_PID,    
state_size : 0,    
num_timers : 0,
num_sub_func : 0,   
num_prov_func : 2,   
module_handler: cc1k_radio_handler, 
funct : {
  {radio_control, "cCw2", RADIO_PID, SENSOR_CONTROL_FID},
  {radio_data_ready_cb, "cCS3", RADIO_PID, SENSOR_DATA_READY_FID},
},   
};

static sos_module_t cc1k_module;
//-----------------------------------------------------------------------------
// RSSI ADC CALLBACK
//static inline void RSSIADC_dataReady(uint16_t data);
// defined in cc1k.h

static inline void WakeupTimer_fired();  //!< Wakeup Timer Handler
static inline void SquelchTimer_fired(); //!< Squelch Timer Handler

static inline int16_t MacBackoff_congestionBackoff();
static inline int16_t MacBackoff_initialBackoff();

//-----------------------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
static inline void adjustSquelch();


//-----------------------------------------------------------------------------
// RADIO STATE MACHINE
enum {
  TX_STATE,
  DISABLED_STATE,
  IDLE_STATE,
  PRETX_STATE,
  SYNC_STATE,
  RX_STATE,
  SENDING_ACK,
  POWER_DOWN_STATE,
  NULL_STATE
};

//-----------------------------------------------------------------------------
// RADIO TX STATE MACHINE
enum {
  TXSTATE_WAIT,
  TXSTATE_START,
  TXSTATE_PREAMBLE,
  TXSTATE_SYNC,
  TXSTATE_PREHEADER,
  TXSTATE_HEADER,
  TXSTATE_DATA,
  TXSTATE_CRC,
  TXSTATE_FLUSH,
  TXSTATE_WAIT_FOR_ACK,
  TXSTATE_READ_ACK,
  TXSTATE_DONE
};

//-----------------------------------------------------------------------------
// RADIO RX STATE MACHINE
enum {
  RXSTATE_PREHEADER,
  RXSTATE_HEADER,
  RXSTATE_DATA,
  RXSTATE_CRC_BYTE1,
  RXSTATE_CRC_BYTE2
};

//-----------------------------------------------------------------------------
// RADIO CONSTANTS
enum {
  SYNC_BYTE    =  0x33,
  NSYNC_BYTE   =  0xcc,
  SYNC_WORD    =  0x33cc,
  NSYNC_WORD   =  0xcc33,
  ACK_LENGTH   =  16,
  MAX_ACK_WAIT =  18
};


//-----------------------------------------------------------------------------
// RADIO VARIABLES
static uint8_t ack_code[3] = {0xab, 0xba, 0x83};
static uint8_t RadioState;      //!< Radio State
static uint8_t RadioTxState;    //!< Radio Transmit Substate
static uint8_t RadioRxState;    //!< Radio Receive Substate
static uint8_t RSSIInitState;
static uint8_t iRSSIcount;
static uint8_t iSquelchCount;
static uint8_t txbufptr_ack;
static Message *txmsgptr;       //!< pointer to transmit buffer
static Message *rxmsg;          //!< pointer to receive buffer
static uint8_t *rxmsg_dataptr;  //!< Pointer to the data buffer of the receive buffer
static uint8_t NextTxByte;
static uint8_t lplpower;        //!< Low power listening mode
static uint8_t lplpowertx;      //!< Low power listening transmit mode
static uint16_t preamblelen;    //!< Current length of the preamble
static uint16_t PreambleCount;  //!< Found a valid preamble
static uint8_t SOFCount;
static uint16_t search_word;
static union {
  uint16_t W;
  struct {
	uint8_t LSB;
	uint8_t MSB;
  };
} RxShiftBuf;
static uint8_t RxBitOffset;	    //!< Bit offset for spibus
static uint16_t RxByteCnt;	    //!< Received byte counter
static uint16_t TxByteCnt;
static uint16_t RSSISampleFreq; //!< In Bytes rcvd per sample
static struct {
	uint8_t bInvertRxData :1;
	uint8_t bTxPending :1;
	uint8_t bTxBusy :1;
	uint8_t bAckEnable :1;
	uint8_t bTimeStampEnable :1;
} bFlag;
static uint16_t usRunningCRC;   //!< Running CRC variable
static uint16_t usRSSIVal;      //!< Suppress nesc warnings
static uint16_t usSquelchVal;
static uint16_t usTempSquelch;
static uint8_t usSquelchIndex;
static uint16_t usSquelchTable[CC1K_SquelchTableSize];
static int16_t sMacDelay;       //!< MAC delay for the next transmission
static uint16_t RecvPktCRC;
static mq_t pq;

//! timestamp of both outgoing and incoming messages.
static uint32_t current_timestamp;
static uint32_t prev_timestamp;
static uint32_t recv_timestamp;

//! timers
static sos_timer_t wakeup_timer;// = TIMER_INITIALIZER(RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);
static sos_timer_t squelch_timer;// = TIMER_INITIALIZER(RADIO_PID, SQUELCH_TIMER_TID, TIMER_REPEAT);

//-----------------------------------------------------------------------------
// RADIO HANDLER
static int8_t cc1k_radio_handler(void *state, Message *e)
{
   MsgParam *p = (MsgParam*)(e->data);
   switch(e->type){
      case MSG_CC1K_RADIO_ADJUST_SQUELCH:
      {
         adjustSquelch();
         return SOS_OK;
      }
      case MSG_TIMER_TIMEOUT:
      {
         MsgParam* params = (MsgParam*)(e->data);
         if(params->byte == WAKEUP_TIMER_TID) {
            WakeupTimer_fired();
         } else /* if (p->byte == SQUELCH_TIMER_TID) */ {
            SquelchTimer_fired();
         }
         return SOS_OK;
      }
      case MSG_DATA_READY:
      {
         RSSIADC_dataReady(p->word);
         return SOS_OK;
      }
      case MSG_RADIO_ENABLE_ACK:
      {
         if(p->byte == true) {
            bFlag.bAckEnable = TRUE;
         } else {
            bFlag.bAckEnable = FALSE;
         }
         return SOS_OK;
         
      }
      default:
         return -EINVAL;
   }
   return SOS_OK;
}


//-----------------------------------------------------------------------------
// RADIO CONTROL FUNC

static int8_t radio_control(func_cb_ptr cb, uint8_t cmd, void *data)
{
  return -EINVAL;
}

//-----------------------------------------------------------------------------
// RADIO GET DATA FUNC
int8_t radio_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags)
{
  RSSIADC_dataReady(value);
  return SOS_OK;
}


//-----------------------------------------------------------------------------
// RADIO SQUELCH ADJUST TASK
static inline void adjustSquelch() {
  uint16_t tempArray[CC1K_SquelchTableSize];
  char i,j,min; 
  uint16_t min_value;
  uint32_t tempsquelch;

  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  {
	usSquelchTable[usSquelchIndex] = usTempSquelch;
	usSquelchIndex++;
	if (usSquelchIndex >= CC1K_SquelchTableSize)
	  usSquelchIndex = 0;
	if (iSquelchCount <= CC1K_SquelchCount)
	  iSquelchCount++;
  }  
  LEAVE_CRITICAL_SECTION();
  
  for (i=0; i<CC1K_SquelchTableSize; i++) {
	tempArray[(int)i] = usSquelchTable[(int)i];
  }
  
  min = 0;
  //  for (j = 0; j < ((CC1K_SquelchTableSize) >> 1); j++) {
  for (j = 0; j < 3; j++) {
	for (i = 1; i < CC1K_SquelchTableSize; i++) {
	  if ((tempArray[(int)i] != 0xFFFF) && 
		  ((tempArray[(int)i] > tempArray[(int)min]) ||
		   (tempArray[(int)min] == 0xFFFF))) {
		min = i;
	  }
	}
	min_value = tempArray[(int)min];
	tempArray[(int)min] = 0xFFFF;
  }
  tempsquelch = ((uint32_t)(usSquelchVal << 5) + (uint32_t)(min_value << 1));
  ENTER_CRITICAL_SECTION();
  usSquelchVal = (uint16_t)((tempsquelch / 34) & 0x0FFFF);
  LEAVE_CRITICAL_SECTION();
}


//-----------------------------------------------------------------------------
// RADIO INITIALIZE
int8_t radio_init(){
	cc1k_radio_init();
	cc1k_radio_start();
	return SOS_OK;
}


int8_t cc1k_radio_init(){
  char i;
  HAS_CRITICAL_SECTION;

  SET_CC_PALE_DD_OUT();          
  SET_CC_PDATA_DD_OUT();
  SET_CC_PCLK_DD_OUT();
  SET_MISO_DD_IN(); 
  SET_OC1C_DD_IN();

  ENTER_CRITICAL_SECTION();
  RadioState = DISABLED_STATE;
  RadioTxState = TXSTATE_PREAMBLE;
  RadioRxState = RXSTATE_HEADER;
  RSSIInitState = NULL_STATE;
  rxmsg = NULL; 
  RxBitOffset = 0;
  iSquelchCount = 0;
  PreambleCount = 0;
  RSSISampleFreq = 0;
  RxShiftBuf.W = 0;
  iRSSIcount = 0;
  bFlag.bTxPending = FALSE;
  bFlag.bTxBusy = FALSE;
  bFlag.bAckEnable = FALSE;
  bFlag.bTimeStampEnable = 0;
  sMacDelay = -1;
  usRSSIVal = -1;
  usSquelchIndex = 0;
  lplpower = lplpowertx = 0;
  usSquelchVal = CC1K_SquelchInit;
  LEAVE_CRITICAL_SECTION();
  for (i = 0; i < CC1K_SquelchTableSize; i++)
	usSquelchTable[(int)i] = CC1K_SquelchInit;	
  spi_init(); // set spi bus to slave mode
  cc1k_cnt_init();
  cc1k_cnt_SelectLock(0x9);		// Select MANCHESTER VIOLATION
  bFlag.bInvertRxData = cc1k_cnt_GetLOStatus();
  sched_register_kernel_module(&cc1k_module, sos_get_header_address(mod_header), NULL);
  ker_adc_proc_bindPort(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_HW_PORT, RADIO_PID, SENSOR_DATA_READY_FID);
  // Timer needs to be done after reigsteration
  ker_permanent_timer_init(&wakeup_timer, RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);
  ker_permanent_timer_init(&squelch_timer, RADIO_PID, SQUELCH_TIMER_TID, TIMER_REPEAT);
  //! Initialize sending queue
  mq_init(&pq);
  return SOS_OK;
}

//-----------------------------------------------------------------------------
// SQUELCH LOGIC TIMER HANDLER
static inline void SquelchTimer_fired() 
{
  char currentRadioState;
  HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
  currentRadioState = RadioState;
  if (currentRadioState == IDLE_STATE){
	RSSIInitState = currentRadioState;
	LEAVE_CRITICAL_SECTION();
	ker_adc_proc_getData(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_FLAGS);
  } else {
	LEAVE_CRITICAL_SECTION();
  }
  return;
}

//-----------------------------------------------------------------------------
// WAKEUP LOGIC TIMER HANDLER
static inline void WakeupTimer_fired() 
{
  uint16_t sleeptime;
  bool tempTxPending;
  uint8_t currentRadioState;
  HAS_CRITICAL_SECTION;
  if (lplpower == 0)
	return;
  ENTER_CRITICAL_SECTION();
  currentRadioState = RadioState;
  tempTxPending = bFlag.bTxPending;
  LEAVE_CRITICAL_SECTION();
  switch(currentRadioState) {
  case IDLE_STATE:
	sleeptime = ((pgm_read_byte(&CC1K_LPL_SleepTime[lplpower*2]) << 8) |
				 pgm_read_byte(&CC1K_LPL_SleepTime[(lplpower*2)+1]));
	if (!tempTxPending) {
	  ENTER_CRITICAL_SECTION();
	  RadioState = POWER_DOWN_STATE;
	  LEAVE_CRITICAL_SECTION();
	  ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, sleeptime);
	  ker_timer_stop(RADIO_PID, SQUELCH_TIMER_TID);
	  cc1k_cnt_stop();        
	  spi_disableIntr(); 
	}
	else {
	  ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, CC1K_LPL_PACKET_TIME*2);
	}
	break;
  case POWER_DOWN_STATE:
	sleeptime = pgm_read_byte(&CC1K_LPL_SleepPreamble[lplpower]);
	ENTER_CRITICAL_SECTION();
	RadioState = IDLE_STATE;
	LEAVE_CRITICAL_SECTION();
	cc1k_cnt_start(); 
	cc1k_cnt_BIASOn();
	spi_rxmode();
	cc1k_cnt_RxMode();
	spi_enableIntr();
	if (iSquelchCount > CC1K_SquelchCount) {
	  ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalSlow); 
	} else {
	  ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalFast);
	}
	ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, sleeptime);
	break;
  default:
	ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, CC1K_LPL_PACKET_TIME*2);
  }
  return;
}

//-----------------------------------------------------------------------------
// RADIO STOP
int8_t cc1k_radio_stop() {
  HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
  RadioState = DISABLED_STATE;
  LEAVE_CRITICAL_SECTION();
  ker_timer_stop(RADIO_PID, SQUELCH_TIMER_TID);
  ker_timer_stop(RADIO_PID, WAKEUP_TIMER_TID);
  cc1k_cnt_stop();
  spi_disableIntr();
  return SOS_OK;
}

//-----------------------------------------------------------------------------
// RADIO START
int8_t cc1k_radio_start() {
  uint8_t currentRadioState;
  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  currentRadioState = RadioState;
  LEAVE_CRITICAL_SECTION();
  
  if (currentRadioState == DISABLED_STATE) {
	ENTER_CRITICAL_SECTION();
	RadioState  = IDLE_STATE;
	bFlag.bTxPending = bFlag.bTxBusy = FALSE;
	sMacDelay = -1;
	preamblelen = ((pgm_read_byte(&CC1K_LPL_PreambleLength[lplpowertx*2]) << 8) |
				   pgm_read_byte(&CC1K_LPL_PreambleLength[(lplpowertx*2)+1]));
	LEAVE_CRITICAL_SECTION();
	if (lplpower == 0) {
	  // all power on, captain!
	  ENTER_CRITICAL_SECTION();
	  RadioState = IDLE_STATE;
	  LEAVE_CRITICAL_SECTION();
	  cc1k_cnt_start();
	  cc1k_cnt_BIASOn();
	  spi_rxmode();
	  cc1k_cnt_RxMode();	  
	  if (iSquelchCount > CC1K_SquelchCount)
		ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalSlow);
	  else
		ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalFast);
	  spi_enableIntr(); //Enable SPI Interrupt
	}
	else {
	  uint16_t sleeptime = ((pgm_read_byte(&CC1K_LPL_SleepTime[lplpower*2]) << 8) |
							pgm_read_byte(&CC1K_LPL_SleepTime[(lplpower*2)+1]));
	  ENTER_CRITICAL_SECTION();
	  RadioState = POWER_DOWN_STATE;
	  LEAVE_CRITICAL_SECTION();	  
	  ker_timer_stop(RADIO_PID, SQUELCH_TIMER_TID);
	  ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, sleeptime);
	}
  }
  rxmsg = msg_create();
  return SOS_OK;
}

//-----------------------------------------------------------------------------
// RADIO SEND PACKET
void radio_msg_alloc(Message *m)
{
  //register uint8_t tmp;
  uint8_t currentRadioState = 0;
  HAS_CRITICAL_SECTION;
  if(flag_msg_release(m->flag)){
	ker_change_own(m->data, RADIO_PID);
  }
  ENTER_CRITICAL_SECTION();
  currentRadioState = RadioState;
  if (bFlag.bTxBusy) {
	mq_enqueue(&pq, m);
	LEAVE_CRITICAL_SECTION();
	return;
  }
  /* no message in the buffer */
  bFlag.bTxBusy = TRUE;
  // initially back off [1,32] bytes (approx 2/3 packet)
  //  sMacDelay = (ker_rand() & 0x1F) + 1;
  sMacDelay = MacBackoff_initialBackoff();
  bFlag.bTxPending = TRUE;
  txmsgptr = m;
  LEAVE_CRITICAL_SECTION();
  // if we're off, start the radio
  if (currentRadioState == POWER_DOWN_STATE) {
	// disable wakeup timer
	ker_timer_stop(RADIO_PID, WAKEUP_TIMER_TID);
	cc1k_cnt_start(); 
	cc1k_cnt_BIASOn(); 
	cc1k_cnt_RxMode(); 
	spi_rxmode();       //!< SPI to miso
	spi_enableIntr();   //!< enable spi interrupt
	if (iSquelchCount > CC1K_SquelchCount){
	  ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalSlow);
	} else {
	  ker_timer_restart(RADIO_PID, SQUELCH_TIMER_TID, CC1K_SquelchIntervalFast);
	}
	ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, CC1K_LPL_PACKET_TIME*2); 
	ENTER_CRITICAL_SECTION();
	RadioState = IDLE_STATE;
	LEAVE_CRITICAL_SECTION();
  }
  return;
}


//-----------------------------------------------------------------------------
// RADIO SPI INTERRUPT
int8_t cc1k_radio_spi_interrupt(uint8_t data_in){
  if (bFlag.bInvertRxData) 
	data_in = ~data_in;
  prev_timestamp = current_timestamp;
  current_timestamp = ker_systime32();
  switch (RadioState) {
  case TX_STATE:  // RadioState
	{
	  spi_writeByte(NextTxByte);
	  TxByteCnt++;
	  switch (RadioTxState) {
	  case TXSTATE_PREAMBLE:
		if (!(TxByteCnt < preamblelen)) {
		  NextTxByte = SYNC_BYTE;
		  RadioTxState = TXSTATE_SYNC;
		}
		break;		
	  case TXSTATE_SYNC:
		NextTxByte = NSYNC_BYTE;
		RadioTxState = TXSTATE_PREHEADER;
		TxByteCnt = -1;
		break;
	  case TXSTATE_PREHEADER:
		// This state has been added to transmit the Group ID etc.
		// Add timestamp here as we just transmitted sync word
		if(txmsgptr->type == MSG_TIMESTAMP)
		{
			memcpy(txmsgptr->data, (uint8_t*)(&current_timestamp),sizeof(uint32_t));
		}

		NextTxByte = node_group_id;
		RadioTxState = TXSTATE_HEADER;
		TxByteCnt = -1;
		break;
	  case TXSTATE_HEADER:
		if ((uint8_t)(TxByteCnt) < SOS_MSG_HEADER_SIZE) {
		  uint8_t *byte = (uint8_t*)txmsgptr;
		  //! send header
		  NextTxByte = byte[(TxByteCnt)];
		  usRunningCRC = crcByte(usRunningCRC,NextTxByte);
		} else {
		  //! send first byte of larg
		  if(txmsgptr->len != 0){
			NextTxByte = txmsgptr->data[0];
			usRunningCRC = crcByte(usRunningCRC,NextTxByte);
			TxByteCnt = 0;
			RadioTxState = TXSTATE_DATA;
		  } else {
			NextTxByte = (uint8_t)(usRunningCRC);
			RadioTxState = TXSTATE_CRC;
		  }
		}
		break;
	  case TXSTATE_DATA:
		if (((uint8_t)(TxByteCnt)) < txmsgptr->len) {
		  //! send bytes in larg
		  NextTxByte = txmsgptr->data[(TxByteCnt)];
		  usRunningCRC = crcByte(usRunningCRC,NextTxByte);
		} else {
		  NextTxByte = (uint8_t)(usRunningCRC);
		  RadioTxState = TXSTATE_CRC;
		}
		break;
	  case TXSTATE_CRC:
		NextTxByte = (uint8_t)(usRunningCRC>>8);
		RadioTxState = TXSTATE_FLUSH;
		TxByteCnt = 0;
		break;
	  case TXSTATE_FLUSH:
		if (TxByteCnt > 3) {
		  if ((bFlag.bAckEnable == TRUE) && (txmsgptr->daddr != BCAST_ADDRESS)) {
			TxByteCnt = 0;
			RadioTxState = TXSTATE_WAIT_FOR_ACK;
		  } else {
			txbufptr_ack = 1;
			RadioTxState = TXSTATE_DONE;
		  }
		}
		break;
	  case TXSTATE_WAIT_FOR_ACK:
		if(TxByteCnt == 1){
		  spi_disableIntr();
		  ENABLE_GLOBAL_INTERRUPTS();
		  spi_rxmode(); 
		  cc1k_cnt_RxMode();
		  DISABLE_GLOBAL_INTERRUPTS();
		  spi_enableIntr();
		}
		if (TxByteCnt > 3) {
		  RadioTxState = TXSTATE_READ_ACK;
		  TxByteCnt = 0;
		  search_word = 0;
		}
		break;
	  case TXSTATE_READ_ACK:
		{
		  uint8_t i;
		  for(i = 0; i < 8; i ++){
			search_word <<= 1;
			if(data_in & 0x80) search_word |=  0x1;
			data_in <<= 1;
			if (search_word == 0xba83){
			  txbufptr_ack = 1;
			  RadioTxState = TXSTATE_DONE;
			  return SOS_OK;
			}
		  }
		}
		if(TxByteCnt == MAX_ACK_WAIT){
		  txbufptr_ack = 0;
		  RadioTxState = TXSTATE_DONE;
		}
		break;
	  case TXSTATE_DONE:
	  default:
		{
		  bFlag.bTxPending = FALSE;		  
		  if(bFlag.bTimeStampEnable) {
			  timestamp_outgoing(txmsgptr, current_timestamp);
		  }
		  msg_send_senddone(txmsgptr, txbufptr_ack, RADIO_PID);
		  txmsgptr = mq_dequeue(&pq);
		  // Check the packet queue
		  if(txmsgptr) {
			// Check if packet queue has elements to send
			sMacDelay = MacBackoff_initialBackoff();
			bFlag.bTxPending = TRUE;
		  } else {
			bFlag.bTxBusy = FALSE;
		  }
		  spi_disableIntr();
		  ENABLE_GLOBAL_INTERRUPTS();
		  spi_rxmode(); 
		  cc1k_cnt_RxMode();           
		  DISABLE_GLOBAL_INTERRUPTS();
		  spi_enableIntr();
		  RadioState = IDLE_STATE;
		  RSSIInitState = RadioState;
		  ker_adc_proc_getData(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_FLAGS);
		  break;
		}
	  }
	}
	break;
	
  case DISABLED_STATE:  // RadioState
	break;
	
  case IDLE_STATE:  // RadioState
	{
	  if (((data_in == (0xaa)) || (data_in == (0x55)))) {
		PreambleCount++;
		if (PreambleCount > CC1K_ValidPrecursor) {
		  PreambleCount = SOFCount = 0;
		  RxBitOffset = RxByteCnt = 0;
		  usRunningCRC = 0;
		  if(rxmsg != NULL) {
			RadioState = SYNC_STATE;
		  }
		  else {
			//! Lets try and allocate a message right now
			rxmsg = msg_create();
			if (rxmsg != NULL)
			  RadioState = SYNC_STATE;
		  }  
		}
	  }
	  else if ((bFlag.bTxPending) && (--sMacDelay <= 0)) {
		RadioState = PRETX_STATE;
		RSSIInitState = PRETX_STATE;
		iRSSIcount = 0;
		PreambleCount = 0;
		ker_adc_proc_getData(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_FLAGS);
	  }
	}
	break;
	
  case PRETX_STATE:   // RadioState
	{
	  if (((data_in == (0xaa)) || (data_in == (0x55)))) {
		// Back to the penalty box.
		sMacDelay = MacBackoff_congestionBackoff();
		RadioState = IDLE_STATE;
	  }
	}
	break;

  case SYNC_STATE:  // RadioState
	{
	  // draw in the preamble bytes and look for a sync byte
	  // save the data in a short with last byte received as msbyte
	  //    and current byte received as the lsbyte.
	  // use a bit shift compare to find the byte boundary for the sync byte
	  // retain the shift value and use it to collect all of the packet data
	  // check for data inversion, and restore proper polarity 
	  // XXX-PB: Don't do this.
	  uint8_t i;
	  if ((data_in == 0xaa) || (data_in == 0x55)) {
		// It is actually possible to have the LAST BIT of the incoming
		// data be part of the Sync Byte.  SO, we need to store that
		// However, the next byte should definitely not have this pattern.
		// XXX-PB: Do we need to check for excessive preamble?
		RxShiftBuf.MSB = data_in;
	  } else {
		// TODO: Modify to be tolerant of bad bits in the preamble...
		uint16_t usTmp;
		switch (SOFCount) {
		case 0:
		  RxShiftBuf.LSB = data_in;
		  break;
		case 1:
		case 2: 
		  // bit shift the data in with previous sample to find sync
		  usTmp = RxShiftBuf.W;
		  RxShiftBuf.W <<= 8;
		  RxShiftBuf.LSB = data_in;
		  
		  for(i=0;i<8;i++) {
			usTmp <<= 1;
			if(data_in & 0x80)
			  usTmp  |=  0x1;
			data_in <<= 1;
			// check for sync bytes
			if (usTmp == SYNC_WORD) {
				//! add timestamp here as we just received sync word
			  RadioState = RX_STATE;
			  RadioRxState = RXSTATE_PREHEADER;
			  RSSIInitState = RX_STATE;
			  ker_adc_proc_getData(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_FLAGS);
			  RxBitOffset = 7-i;
			  break;
			}
		  }
		  break;
		default:
		  RadioState = IDLE_STATE;  // Ensures we wait till the end of the transmission
		  break;
		}
		SOFCount++;
	  }
	}
	break;

	//  collect the data and shift into double buffer
	//  shift out data by correct offset
	//  invert the data if necessary
	//  stop after the correct packet length is read
	//  return notification to upper levels
	//  go back to idle state
  case RX_STATE:   // RadioState
	{
	  char Byte;
	  RxShiftBuf.W <<=8;
	  RxShiftBuf.LSB = data_in;  
	  Byte = (RxShiftBuf.W >> RxBitOffset);
	  switch (RadioRxState){
		//! Receive PreHeader
	  case RXSTATE_PREHEADER:
		{
		  //! Currently only receives the group ID of the node
		  if (Byte == node_group_id){
			recv_timestamp = prev_timestamp + ( ( (current_timestamp - prev_timestamp)/8 ) * (7-RxBitOffset));
			RadioRxState = RXSTATE_HEADER;
		  }
		  else{
			RadioState = IDLE_STATE;
			return SOS_OK;
		  }
		  break;
		}
		//! Receive Header
	  case RXSTATE_HEADER:
		{
		  ((char*)rxmsg)[(int)RxByteCnt] = Byte;
		  RxByteCnt++;
		  usRunningCRC = crcByte(usRunningCRC, Byte);
		  if (RxByteCnt == SOS_MSG_HEADER_SIZE){
			//! If we have an empty data field, we skip over to the Rx CRC state
			if (rxmsg->len == 0){
			  rxmsg->data = NULL;
			  rxmsg->flag = 0;
			  RadioRxState = RXSTATE_CRC_BYTE1;
			  return SOS_OK;
			}
			if (rxmsg->len > 128){
			  //! Filter out the packet if it has a large length
			  RadioState = IDLE_STATE;
			  return SOS_OK;
			}			  
			rxmsg->data = ker_malloc(rxmsg->len, RADIO_PID);
			if (rxmsg->data == NULL){
			  //! There is no memory to receive the packet anyway so dump it !!
			  RadioState = IDLE_STATE;
			  return SOS_OK;
			}
			rxmsg->flag = SOS_MSG_RELEASE;
			rxmsg_dataptr = rxmsg->data;
			RxByteCnt = 0;
			RadioRxState = RXSTATE_DATA;
		  }
		  break;
		}
		//! Receive Packet Data
	  case RXSTATE_DATA:
		{
		  ((char*)rxmsg_dataptr)[(int)RxByteCnt] = Byte;
		  RxByteCnt++;
		  usRunningCRC = crcByte(usRunningCRC, Byte);
		  if (RxByteCnt == rxmsg->len){
			RadioRxState = RXSTATE_CRC_BYTE1;
		  }
		  break;
		}
		//! Receive First CRC Byte
	  case RXSTATE_CRC_BYTE1:
		{
		  RecvPktCRC = (uint8_t)Byte;
		  RadioRxState = RXSTATE_CRC_BYTE2;
		  break;
		}
		//! Finished receiving the packet
	  case RXSTATE_CRC_BYTE2:
		{
		  RecvPktCRC = (((uint16_t) (Byte)) << 8) | RecvPktCRC;

		  if (RecvPktCRC == usRunningCRC) {
			//! Here CRC has passed
			if (bFlag.bAckEnable == TRUE){
			  if (rxmsg->daddr == node_address) {
				RadioState = SENDING_ACK;
				spi_disableIntr();
				ENABLE_GLOBAL_INTERRUPTS();
				cc1k_cnt_TxMode();
				spi_txmode();
				DISABLE_GLOBAL_INTERRUPTS();
				spi_enableIntr();
				spi_writeByte(0xaa);
				RxByteCnt = 0;
				return SOS_OK; 
			  }
			}
			spi_disableIntr();
			//! Here we are handling the promiscous packet or packets that do not require ack
			RadioState = IDLE_STATE;
			if(rxmsg->type == MSG_TIMESTAMP)
			{
				memcpy(&rxmsg->data[4], (uint8_t *)(&recv_timestamp), sizeof(uint32_t));
			}
			
			handle_incoming_msg(rxmsg, SOS_MSG_RADIO_IO);
			rxmsg = msg_create();
		  }
		  else {
			//! CRC has failed here
			// We won't pass bad CRC packet.  -- Simon
			spi_disableIntr();
			ker_free(rxmsg->data);
			RadioState = IDLE_STATE; 
		  }
		  spi_enableIntr();
		  break;
		}
	  default:
		break;
	  }
	}
	break;

  case SENDING_ACK:  // RadioState
	{
	  RxByteCnt++;
	  if (RxByteCnt >= ACK_LENGTH) { 
		// Here CRC has passed and we have finished sending the ACK
		spi_disableIntr(); 
		ENABLE_GLOBAL_INTERRUPTS();
		cc1k_cnt_RxMode();           
		spi_rxmode(); 
		DISABLE_GLOBAL_INTERRUPTS();
		RadioState = IDLE_STATE; 
		if(bFlag.bTimeStampEnable) {
			timestamp_incoming(rxmsg, current_timestamp);
		}
		handle_incoming_msg(rxmsg, SOS_MSG_RADIO_IO);
		rxmsg = msg_create();
		spi_enableIntr();
	  } else if(RxByteCnt >= ACK_LENGTH - sizeof(ack_code) - 2){
		spi_writeByte(ack_code[RxByteCnt + sizeof(ack_code) + 2 - ACK_LENGTH]);
	  }
	}
	break;
	
  default:
	break;
  }
  return SOS_OK;
}

//-----------------------------------------------------------------------------
// RSSI HANDLER
//static inline void RSSIADC_dataReady(uint16_t data) {
void RSSIADC_dataReady(uint16_t data) {
  uint8_t currentRadioState;
  uint8_t initRSSIState;
  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  currentRadioState = RadioState;
  initRSSIState = RSSIInitState;
  LEAVE_CRITICAL_SECTION();
  // find the maximum RSSI value over CC1K_MAX_RSSI_SAMPLES
  switch(currentRadioState) {
  case IDLE_STATE:
	if (initRSSIState == IDLE_STATE){
	  ENTER_CRITICAL_SECTION();
	  usTempSquelch = data;
	  LEAVE_CRITICAL_SECTION();
	  post_short(RADIO_PID, RADIO_PID, MSG_CC1K_RADIO_ADJUST_SQUELCH, 0, 0, 0);
	}
	ENTER_CRITICAL_SECTION();
	RSSIInitState = NULL_STATE;
	LEAVE_CRITICAL_SECTION();
	break;
  case RX_STATE:
	if (initRSSIState == RX_STATE){
	  ENTER_CRITICAL_SECTION();
	  usRSSIVal = data;
	  LEAVE_CRITICAL_SECTION();
	}
	ENTER_CRITICAL_SECTION();
	RSSIInitState = NULL_STATE;
	LEAVE_CRITICAL_SECTION();
	break;
	
  case PRETX_STATE:
	iRSSIcount++;
	// if the channel is clear, GO GO GO!
	if ((data > (usSquelchVal + CC1K_SquelchBuffer)) && (initRSSIState == PRETX_STATE)) { 
	  spi_writeByte(0xaa);
	  spi_disableIntr();
	  ENABLE_GLOBAL_INTERRUPTS();
	  cc1k_cnt_TxMode();
	  spi_txmode(); 
	  DISABLE_GLOBAL_INTERRUPTS();
	  spi_enableIntr();
	  ENTER_CRITICAL_SECTION();
	  {
		usRSSIVal = data;
		iRSSIcount = CC1K_MaxRSSISamples;
		TxByteCnt = 0;
		usRunningCRC = 0;
		RadioState = TX_STATE;
		RadioTxState = TXSTATE_PREAMBLE;
		NextTxByte = 0xaa;
		RSSIInitState = NULL_STATE;
	  }
	  LEAVE_CRITICAL_SECTION();
	  return;
	}
	ENTER_CRITICAL_SECTION();
	RSSIInitState = NULL_STATE;
	LEAVE_CRITICAL_SECTION();
    if (iRSSIcount == CC1K_MaxRSSISamples) {
      ENTER_CRITICAL_SECTION();
	  sMacDelay = MacBackoff_congestionBackoff();
	  RadioState = IDLE_STATE;
      LEAVE_CRITICAL_SECTION();
    }
    else {
	  ENTER_CRITICAL_SECTION();
      RSSIInitState = currentRadioState;
	  LEAVE_CRITICAL_SECTION();
	  ker_adc_proc_getData(MICA2_CC_RSSI_SID, MICA2_CC_RSSI_FLAGS);
    }
    break;
	
  default:
	break;
  }
  return;
}

//-----------------------------------------------------------------------------
// MAC BACKOFF
static inline int16_t MacBackoff_initialBackoff()
{
  return ((ker_rand() & 0x1F) + TIMER_MIN_INTERVAL);
}

static inline int16_t MacBackoff_congestionBackoff()
{
  return ((ker_rand() & 0xF) + TIMER_MIN_INTERVAL);
}


//-----------------------------------------------------------------------------
// RADIO LINK LAYER ACK
void ker_radio_ack_enable()
{
  bFlag.bAckEnable = TRUE;
}

void ker_radio_ack_disable()
{
 bFlag.bAckEnable = FALSE;
}

int8_t radio_set_timestamp(bool on)
{
	bFlag.bTimeStampEnable = on;
	return SOS_OK;
}
