#ifndef _BLINK_H_
#define _BLINK_H_

#ifndef _MODULE_
int8_t blink_init ();
#endif

#endif

//--------------------------------------
// MODULE MESSAGES
//-------------------------------------------------------------
enum
{
  MSG_BEACON_SEND = MOD_MSG_START,	//! beacon task
  MSG_BEACON_PKT = MOD_MSG_START + 1,	//! beacon packet type
  MSG_TR_DATA_PKT = MOD_MSG_START + 2,	//! beacon packet type
  MSG_GET_SERVO
};
