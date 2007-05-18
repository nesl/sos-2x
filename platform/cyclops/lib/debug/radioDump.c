/**
 * \file radioDump.c
 * \brief Breaks a buffer into chunks and sends it out over I2C->Radio
 */

#include <sys_module.h>
#include <hardware.h>
#include <image.h>
#include "radioDump.h"

#define LED_DEBUG
#include <led_dbg.h>

//timer_val is the speed regulator for the radio. Lower means that it floods the radio faster.
#define TIMER_VAL 15		/* 
							 * This is set based on my experamentation on
							 * the radio transmition rate with the cyclops
							 * connected to a micaZ. A mica2 will probably
							 * have a different rate
							 */
#define RADIO_DUMP_TID1		1
#define RADIO_DUMP_TID2		2
#define MAX_RESEND_COUNT	10
#define CYCLOPS_NIC_PID		DFLT_APP_ID0+14
#define NIC_ADDRESS			NODE_ADDR-1

//-------------------------------------------------------
// RADIO DUMP STATUS
//-------------------------------------------------------
enum
  {
    RADIO_DUMP_IDLE = 0,
    RADIO_DUMP_ALL = 1,
    RADIO_DUMP_PARTIAL = 2,
  };

//-------------------------------------------------------
// RADIO DUMP STATE
//-------------------------------------------------------
typedef struct
{
	int16_t currImageHandle;
	uint8_t *inputDumpBuffer;
	uint8_t *ptrInDumpBuffer;
	uint16_t bytesLeft;
	uint16_t BytesTxInLastFrame;
	uint8_t currentState;
	uint8_t buffsize;
	uint8_t sendCount;
	uint8_t appID;
	uint16_t imageSize;	
	framCount_t framesNacked;
} radioDump_state_t;


static radioDumpFrame_t radioFrame;

//-------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------
static int8_t radioDump_msg_handler (void *state, Message * msg);
static uint8_t InitRadioDump (radioDump_state_t * s);
static uint8_t FragmentAndSend (radioDump_state_t * s);

//-------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id = RADIO_DUMP_PID,
  .state_size = sizeof (radioDump_state_t),
  .num_sub_func = 0,
  .num_prov_func = 0,
  .module_handler = radioDump_msg_handler,
};



int8_t radioDumpControl_init ()
{
  ker_register_module(sos_get_header_address (mod_header));
  return SOS_OK;
}

//-------------------------------------------------------
// MESSAGE HANDLER
//-------------------------------------------------------
static int8_t
radioDump_msg_handler (void *state, Message * msg)
{
	radioDump_state_t *s = (radioDump_state_t *) state;
	switch (msg->type)
	{
		case MSG_INIT:
		{
			InitRadioDump (s);	      
			break;
		}

		case MSG_FINAL:
		{
			break;
		}

		case MSG_DUMP_BUFFER_TO_RADIO:
		{
			s->appID = msg->sid;
			CYCLOPS_Image* img = (CYCLOPS_Image*)sys_msg_take_data(msg);
			LED_DBG(LED_AMBER_TOGGLE);
			if (RADIO_DUMP_IDLE != s->currentState)
			{
				ker_free_handle(img->imageDataHandle);
				sys_free(img);
				return -EBUSY;
			}		
			s->inputDumpBuffer = (uint8_t *) ker_get_handle_ptr(img->imageDataHandle);
			if (NULL == s->inputDumpBuffer){
				sys_free(img);
				return -EINVAL;
			}
			s->currImageHandle = img->imageDataHandle;
			s->bytesLeft = imageSize(img);
			s->imageSize = imageSize(img);	
			s->ptrInDumpBuffer = s->inputDumpBuffer;
			s->currentState = RADIO_DUMP_ALL;	
			FragmentAndSend(s);
			sys_free(img);
			break;
		}
      
		case MSG_PKT_SENDDONE:
		{	      
			s->sendCount = 0;		
			s->bytesLeft -= s->BytesTxInLastFrame;
			if(s->currentState == RADIO_DUMP_ALL){
				if (s->bytesLeft > 0){
					sys_timer_start(RADIO_DUMP_TID2, TIMER_VAL, TIMER_ONE_SHOT);	
				} else {    
					s->currentState = RADIO_DUMP_PARTIAL;
					// Kevin - debug 
					// Commenting this section out will make it TCP reliable
					/***************************************************************/
					ker_free_handle(s->currImageHandle);
					InitRadioDump(s);			
					sys_post_value(s->appID, MSG_RADIO_DUMP_DONE, SOS_OK, 0);
					/****************************************************************/	
				}
			} else if ((s->framesNacked.frameSequence[0] > 0) || 
					(s->framesNacked.frameSequence[1] > 0)) {
				sys_timer_start(RADIO_DUMP_TID2, TIMER_VAL, TIMER_ONE_SHOT);	
			}
			break;
		}
      
		case MSG_TIMER_TIMEOUT:
		{				
			MsgParam *param = (MsgParam*) (msg->data);
			switch (param->byte)
			{
				case RADIO_DUMP_TID2:
				{									
					FragmentAndSend(s);										
					break;	
				}	  
			}		
			break;		  
		}   

		case MSG_DATA_ACK:
		{	      
			uint8_t ackback = SOS_OK;

			if ( s->currentState != RADIO_DUMP_IDLE)
			{
				ker_free_handle(s->currImageHandle);
				InitRadioDump(s);			
				sys_post_value(s->appID, MSG_RADIO_DUMP_DONE, SOS_OK, 0);
			}

			// Assuming the Cyclops NIC is connected to cyclops using the UART.
			sys_post_uart(CYCLOPS_NIC_PID, MSG_DATA_ACK_BACK, sizeof(uint8_t), 
														&ackback, 0, NIC_ADDRESS);	
			/* 
			// Assuming the Cyclops NIC is connected to cyclops using the I2C.
			sys_post_i2c(CYCLOPS_NIC_PID, MSG_DATA_ACK_BACK, sizeof(uint8_t), 
			 											&ackback, 0, NIC_ADDRESS);
			 */

			break;
		}
            
		case MSG_DATA_NACK:
		{				
			if (s->currentState == RADIO_DUMP_PARTIAL)
			{
				framCount_t* nackedFrames = (framCount_t *)(msg->data);
				s->framesNacked.frameSequence[0] = nackedFrames->frameSequence[0];
				s->framesNacked.frameSequence[1] = nackedFrames->frameSequence[1];
				s->ptrInDumpBuffer = s->inputDumpBuffer;	
				FragmentAndSend(s);	
			}			
			break;
		}

		// Actuated, reset the cyclops	
		case MSG_GET_SERVO:
		{
			// Ram - I do not understand this. So I am commenting it out !
			//asm volatile ("jmp 0x0");
			break; 		
		}
       
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

//-------------------------------------------------------
// INITIALIZE RADIO DUMP
//-------------------------------------------------------
static uint8_t InitRadioDump (radioDump_state_t * s)
{
	s->inputDumpBuffer = NULL;
	s->ptrInDumpBuffer = NULL;
	s->bytesLeft = 0;
	s->currentState = RADIO_DUMP_IDLE;
	s->sendCount = 0;
	s->framesNacked.frameSequence[0] = 0xFFFFFFFF;
	s->framesNacked.frameSequence[1] = 0xFFFFFFFF;	
	return SOS_OK;
}

//-------------------------------------------------------
// FRAGMENT AND SEND RADIO BUFFER
//-------------------------------------------------------
static uint8_t FragmentAndSend (radioDump_state_t * s)
{
	LED_DBG(LED_GREEN_TOGGLE);	
	
	if (s->currentState == RADIO_DUMP_ALL) {
		if (s->bytesLeft >= RADIO_PAYLOAD_LEN){
			// Send the next fragment.
			memcpy((uint8_t *)&(radioFrame.payload[0]), s->ptrInDumpBuffer, RADIO_PAYLOAD_LEN);
			radioFrame.seq = (s->bytesLeft) / RADIO_PAYLOAD_LEN - 1;
			s->buffsize = sizeof (radioDumpFrame_t);

			// Assuming the Cyclops NIC is connected to cyclops using I2C.
			//sys_post_i2c(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
			//						&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);

			// Assuming the Cyclops NIC is connected to cyclops using UART.
			sys_post_uart(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
										&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);
			s->ptrInDumpBuffer = s->ptrInDumpBuffer + RADIO_PAYLOAD_LEN;
			s->BytesTxInLastFrame = RADIO_PAYLOAD_LEN;
		} else if (s->bytesLeft > 0) {
			// Send the last remaining fragment.
			s->buffsize = offsetof (radioDumpFrame_t, payload) + s->bytesLeft;
			memcpy((uint8_t *)&(radioFrame.payload[0]), s->ptrInDumpBuffer, 
																s->bytesLeft);			
			radioFrame.seq = s->bytesLeft / RADIO_PAYLOAD_LEN - 1;
			// Assuming the Cyclops NIC is connected to cyclops using I2C.
			//sys_post_i2c(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
			//						&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);

			// Assuming the Cyclops NIC is connected to cyclops using UART.
			sys_post_uart(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
										&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);
			s->BytesTxInLastFrame = s->bytesLeft;
		}
	} else {		
		//PARTIAL  
		//NOTE DOESNT WORK!!!!
		if( s->framesNacked.frameSequence[1] ){	// upper 32 bits
			uint8_t index = 0;
			uint32_t mask = 0x80000000;
			while(index < 32){			// upper 32 bits
				if (s->framesNacked.frameSequence[1] & mask){
					s->framesNacked.frameSequence[1] &= ~mask;
					break;			
				}
				index++;
				mask >>= 1;	
			}
			memcpy((uint8_t *)&(radioFrame.payload[0]), 
					(s->ptrInDumpBuffer + (RADIO_PAYLOAD_LEN * index) ), 
					RADIO_PAYLOAD_LEN);
			radioFrame.seq = (s->imageSize) / RADIO_PAYLOAD_LEN - index - 1;
		} else {	// lower 32 bits
			uint8_t index = 0;
			uint32_t mask = 0x80000000;
			while(index < 32){			// lower 32 bits
				if (s->framesNacked.frameSequence[0] & mask){
					s->framesNacked.frameSequence[0] &= ~mask;
					break;			
				}
				index++;
				mask>>=1;	
			}
			memcpy((uint8_t *)&(radioFrame.payload[0]), 
					(s->ptrInDumpBuffer + (s->imageSize/2) + (RADIO_PAYLOAD_LEN * index) ), 
					RADIO_PAYLOAD_LEN);
			radioFrame.seq = (s->imageSize) / RADIO_PAYLOAD_LEN - 
									(s->imageSize/RADIO_PAYLOAD_LEN/2) - index - 1;		
		}				
		s->buffsize = sizeof (radioDumpFrame_t);
		// Assuming the Cyclops NIC is connected to cyclops using I2C.
		//sys_post_i2c(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
		//						&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);

		// Assuming the Cyclops NIC is connected to cyclops using UART.
		sys_post_uart(CYCLOPS_NIC_PID, MSG_RAW_IMAGE_FRAGMENT, s->buffsize, 
									&radioFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);
	}

	return SOS_OK;
}

