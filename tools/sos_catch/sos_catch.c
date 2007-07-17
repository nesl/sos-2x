/**
 * @file sos_catch.c
 * @breif SOS message subscriber
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include "sos_catch.h"

/**
 * Connects to sossrv.exe and subscribes the catch function to all messages.
 * * To customize behavior, write your own C code that implements a catch
 * function in line with the recv_msg_func_t specs and include that
 * file instead of catch_surge.c (see below).
 */

// Funciton pre-declaration:
int sos_msg_dispatcher();

/*
 * Subscribe a funciton to sos messages. pass it an address and port for
 * sos_srv and a function to handle incomming messages.
 */

int sos_subscribe(const char *address, const char *port, recv_msg_func_t catch)
{
  /*
   * Connect to sossrv and give an error if we can't:
   */
  if (sossrv_connect((char*)address, (char*)port) < 0 || connected != 1) {
    printf("Could not connect to sossrv at %s on port %s.", address, port);
    exit(EXIT_FAILURE);
  }

  /*
   * Set up the catch() function to receive messages.
   * sossrv_recv_msg closes the socket after starting a new thread, so we
   * don't have to worry about cleanup.
   */
  if (sossrv_recv_msg(catch) < 0) {
    printf("Could not subscribe to sossrv messages.");
    exit(EXIT_FAILURE);
  }

  /*
   * Call the sos_msg_dipatcher function to start getting messages.
   */
  sos_msg_dispatcher();

  return 0;
}

//------------------------------------------------------------------
// SOS Message Dispatcher
//------------------------------------------------------------------
// Scavenged from test_sossrv_client.c
// This just activates message reception and then goes into an infinite loop.
int sos_msg_dispatcher()
{
    sossrv_post_msg(
			DFLT_APP_ID0, 
			DFLT_APP_ID0, 
			MSG_PC_TO_MOTE, 
			0, 
			NULL, 
			65534, 
			65535); 
  return 0;
}
