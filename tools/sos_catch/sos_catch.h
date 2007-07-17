/**
 * @file sos_catch.h
 * @breif SOS message subscriber header file
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#ifndef SOS_CATCH_HEADER
#define SOS_CATCH_HEADER

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sossrv_client.h>

/**
 * Connects to sossrv.exe and subscribes the catch function to all messages.
 * To customize behavior, write your own C code that implements a catch
 * function in line with the recv_msg_func_t specs and pass it to the
 * subscribe function.
 */

/*
 * This is from test_sossrv_client.c
 * It's used for posting a message to sos_serve that initiates message
 * handling.
 */
#define MSG_PC_TO_MOTE  32

extern int connected;

/*
 * Subscribe a funciton to sos messages. pass it an address and port for
 * sos_srv and a function to handle incomming messages.
 */

int sos_subscribe(const char *address, const char *port, recv_msg_func_t catch);

#endif // #ifndef SOS_CATCH_HEADER
