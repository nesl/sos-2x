/**
 * \file serialDump.c
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \brief Break up image into chunks and transfer over serial link
 */

#include <module.h>
#include <hardware.h>
#include <image.h>
#include "serialDump.h"
//#define LED_DEBUG
#include <led_dbg.h>

#define CYCLOPS_NIC_PID		DFLT_APP_ID0+14
#define NIC_ADDRESS		NODE_ADDR-1

//-------------------------------------------------------
// SERIAL DUMP STATUS
//-------------------------------------------------------
enum
  {
    SERIAL_DUMP_IDLE = 0,
    SERIAL_DUMP_BUSY = 1,
  };

//-------------------------------------------------------
// SERIAL DUMP STATE
//-------------------------------------------------------
typedef struct
{
  int16_t currImageHandle;
  uint8_t *inputDumpBuffer;
  uint8_t *ptrInDumpBuffer;
  uint16_t bytesLeft;
  uint16_t BytesTxInLastFrame;
  //  serialDumpFrame_t *serialFrame;
  uint8_t currentState;
  uint8_t appID;
} serialDump_state_t;


serialDumpFrame_t serialFrame;

//-------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------
static int8_t serialDump_msg_handler (void *state, Message * msg);
static uint8_t InitSerialDump (serialDump_state_t * s);
static uint8_t FragmentAndSend (serialDump_state_t * s);

//-------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id = SERIAL_DUMP_PID,
  .state_size = sizeof (serialDump_state_t),
  .num_timers = 0,
  .num_sub_func = 0,
  .num_prov_func = 0,
  .module_handler = serialDump_msg_handler,
};



int8_t serialDumpControl_init()
{
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

//-------------------------------------------------------
// MESSAGE HANDLER
//-------------------------------------------------------
static int8_t serialDump_msg_handler (void *state, Message * msg)
{
  serialDump_state_t *s = (serialDump_state_t *) state;
  switch (msg->type)
    {
    case MSG_INIT:
      {
	InitSerialDump (s);
	break;
      }
    case MSG_DUMP_BUFFER_TO_SERIAL:
      {
	CYCLOPS_Image *img;
	img = (CYCLOPS_Image *) (msg->data);
	s->appID = msg->sid;
	if (SERIAL_DUMP_BUSY == s->currentState){
	  ker_free_handle (img->imageDataHandle);
	  return -EBUSY;
	}
	s->inputDumpBuffer =   (uint8_t *) ker_get_handle_ptr (img->imageDataHandle);
	if (NULL == s->inputDumpBuffer) return -EINVAL;
	s->currImageHandle = img->imageDataHandle;
	s->bytesLeft = imageSize(img);
	s->ptrInDumpBuffer = s->inputDumpBuffer;
	s->currentState = SERIAL_DUMP_BUSY;
	FragmentAndSend (s);
	break;
      }
    case MSG_PKT_SENDDONE:
      {
	s->bytesLeft -= s->BytesTxInLastFrame;
	if (s->bytesLeft > 0){
	  FragmentAndSend (s);
	}
	else{
	  ker_free_handle (s->currImageHandle);
	  InitSerialDump (s);
	  post_short (s->appID, SERIAL_DUMP_PID, MSG_SERIAL_DUMP_DONE,
		      SOS_OK, SOS_OK, 0);
	}
	break;
      }

    default:
      {
	return -EINVAL;
      }
    }
  return SOS_OK;
}

//-------------------------------------------------------
// INITIALIZE SERIAL DUMP
//-------------------------------------------------------
static uint8_t InitSerialDump (serialDump_state_t * s)
{
  s->inputDumpBuffer = NULL;
  s->ptrInDumpBuffer = NULL;
  s->bytesLeft = 0;
  s->currentState = SERIAL_DUMP_IDLE;
  return SOS_OK;
}

//-------------------------------------------------------
// FRAGMENT AND SEND SERIAL BUFFER
//-------------------------------------------------------
static uint8_t FragmentAndSend (serialDump_state_t * s)
{
  if (s->bytesLeft >= UART_PAYLOAD_LEN){
    memcpy ((uint8_t *) & (serialFrame.payload[0]), s->ptrInDumpBuffer, UART_PAYLOAD_LEN);
    serialFrame.seq = s->bytesLeft / UART_PAYLOAD_LEN - 1;
    post_uart (CYCLOPS_NIC_PID, SERIAL_DUMP_PID, MSG_RAW_IMAGE_FRAGMENT,
	       sizeof (serialDumpFrame_t), &serialFrame, SOS_MSG_RELIABLE,
	       NIC_ADDRESS);
    s->ptrInDumpBuffer = s->ptrInDumpBuffer + UART_PAYLOAD_LEN;
    s->BytesTxInLastFrame = UART_PAYLOAD_LEN;
  }
  else if (s->bytesLeft > 0){
    uint8_t buffsize = offsetof (serialDumpFrame_t, payload) + s->bytesLeft;
    memcpy ((uint8_t *) & (serialFrame.payload[0]), s->ptrInDumpBuffer,
	    s->bytesLeft);
    serialFrame.seq = s->bytesLeft / UART_PAYLOAD_LEN - 1;
    post_uart (CYCLOPS_NIC_PID, SERIAL_DUMP_PID, MSG_RAW_IMAGE_FRAGMENT,
	       buffsize, &serialFrame, SOS_MSG_RELIABLE, NIC_ADDRESS);
    //    s->BytesTxInLastFrame = (serialFrame.seq + 1) * UART_PAYLOAD_LEN;
    s->BytesTxInLastFrame = s->bytesLeft;
  }
  return SOS_OK;
}
