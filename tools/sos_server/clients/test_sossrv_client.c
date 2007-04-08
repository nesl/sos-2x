/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * \file test_sossrv_client.c
 * \brief Test the sossrv client
 */
#include <stdio.h>
#include <string.h>

#include <sossrv_client.h>
#include <mod_pid.h>


//------------------------------------------------------------------
// CONSTANTS
//------------------------------------------------------------------
#define MSG_PC_TO_MOTE  32

//------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------
static int recvsosmsg(Message* psosmsg);
static int sos_msg_dispatcher();



//------------------------------------------------------------------
int main(int argc, char *argv[]){

	if ((argc != 1) && (argc != 3)){ 
		printf("Usage Error!\n");
		printf("test_sossrv <server address> <server port>\n");
		return -1;
	}

	// Connect to the SOS server
	if (argc == 3) { 
		sossrv_connect(argv[1], argv[2]);
	} else {
		sossrv_connect(DEFAULT_IP_ADDR, DEFAULT_PORT);
	}

  // Setup the handler to receive SOS Messages
  if (sossrv_recv_msg(recvsosmsg) < 0){
    printf("Setting up handler before connecting.\n");
  }

  // Message Dispatcher
  sos_msg_dispatcher();
  return 0;
}

static uint32_t expected_recv_cnt = 0;

static int recvsosmsg(Message* psosmsg)
{
	uint32_t recv_cnt;
	//
	// Message receiver
	//
	recv_cnt = entohl(*(uint32_t*) psosmsg->data);
	if( expected_recv_cnt != recv_cnt ) {
		printf("ERROR: Missing cnt from %d to %d\n", expected_recv_cnt, recv_cnt - 1);
	} else {
		if( expected_recv_cnt % 50 == 0 ) {
			printf("recv %d\n", expected_recv_cnt);
		}
	}
	expected_recv_cnt = recv_cnt + 1;
	/*
  int i;
  printf("Dest Mod Id: %d\n", psosmsg->did);
  printf("Src  Mod Id: %d\n", psosmsg->sid);
  printf("Dest Addr  : %X\n", psosmsg->daddr);
  printf("Src  Addr  : %X\n", psosmsg->saddr);
  printf("Msg Type   : %d\n", psosmsg->type);
  printf("Msg Length : %d\n", psosmsg->len);
  printf("Msg Data: ");
  psosmsg->data[psosmsg->len] = '\0';
  printf("%s\n",psosmsg->data);
	*/
  return 0;
}


//------------------------------------------------------------------
// SOS Message Dispatcher
//------------------------------------------------------------------
static int sos_msg_dispatcher()
{
    sossrv_post_msg(
			DFLT_APP_ID0, 
			DFLT_APP_ID0, 
			MSG_PC_TO_MOTE, 
			0, 
			NULL, 
			65534, 
			65535); 
  while (1){

  }
  return 0;
}
