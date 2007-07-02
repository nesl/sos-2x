/**
 * @file sos_catch.c
 * @breif SOS message subscriber
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sossrv_client.h>

/**
 * Connects to sossrv.exe and subscribes the catch function to all messages.
 * * To customize behavior, write your own C code that implements a catch
 * function in line with the recv_msg_func_t specs and include that
 * file instead of catch_surge.c (see below).
 */

/*
 * Implements the function used to catch messages. Redefine that function
 * in order to change the behavior of sos_catch:
 */

#include "catch_surge.c"

/*
 * Don't ask me why: this is from test_sossrv_client.c
 * It's used for posting a message to sos_serve that initiates message
 * handling.
 */
#define MSG_PC_TO_MOTE  32

extern char* optarg;
extern int connected;

int main(int argc, char *argv[])
{
  char* ADDRESS = "127.0.0.1"; // Default is localhost
  char* PORT = "7915";         // This is the default port
  char c;

  /*
   * Parse the options using getopt:
   */
  while(-1 != (c = getopt(argc, argv, "c:hr:"))) {
    switch (c) {
      case 'c': PORT=optarg; break;
      case 'h': { printf("Usage: sos_dump [-h] [-c PORT] [-r HOST]\n");
                  printf("Flag summary:\n");
                  printf("  -c [PORT]      Connect  Connect to sossrv on port PORT. Default is 7915.\n");
                  printf("  -h           Help       Display this message and quit.");
                  printf("  -r [ADDRESS]   Remote   Connect to sossrv at ADDRESS (a dotted-decimal IP\n");
                  printf("                          address). Default is localhost.\n");
                exit(EXIT_SUCCESS);
              } break; // technically unnecessary
      case 'r': ADDRESS=optarg; break;
    }
  }

  /*
   * Connect to sossrv and give an error if we can't:
   */
  if (sossrv_connect(ADDRESS, PORT) < 0 || connected != 1) {
    printf("Could not connect to sossrv at %s on port %s.", ADDRESS, PORT);
    exit(EXIT_FAILURE);
  }

  /*
   * Set up the catch() function (from catch.c) to receive messages.
   * sossrv_recv_msg closes the socket after starting a new thread, so we
   * don't have to worry about cleanup.
   */
  if (sossrv_recv_msg((recv_msg_func_t)&catch) < 0) {
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
  while (1){
    sleep(1);
  }
  return 0;
}
