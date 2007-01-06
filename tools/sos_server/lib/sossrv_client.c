/**
 * \file sossrv_client.c
 * \brief Library to connect to sossrv
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include <sos_endian.h>
#include <sossrv_client.h>
#include <sossrv.h>
#include <sock_utils.h>


//-----------------------------------------------------------------
// GLOBAL VARIABLES
int clientsockfd = 0;
int connected = 0;
pthread_t sossrv_rx_thread;
recv_msg_func_t sossrv_rx_msg_handler;

//-----------------------------------------------------------------
// STATIC FUNCTION
void *sossrv_rx_thread_handler(void *arg);

//------------------------------------------------------------------
// CONNECT TO SOSSRV
int sossrv_connect(char* server_addr, char* server_port)
{
  struct sockaddr_in server_address;

  // Create the socket
  if ((clientsockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("sossrv_connect: socket");
    exit(EXIT_FAILURE);
  }
  
  // Setup the internet address
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(atoi(server_port));
  inet_aton(server_addr, &(server_address.sin_addr));
  memset(&(server_address.sin_zero), '\0', 8);
  
  DEBUG("Connecting to server at %s\n", inet_ntoa(server_address.sin_addr));
  DEBUG("Connecting to port at %d\n", ntohs(server_address.sin_port));
  
  // Connect
  if (connect(clientsockfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1) {
    perror("sossrv_connect: connect");
    exit(EXIT_FAILURE);
  }
  DEBUG("Established TCP connection ..\n");
  connected = 1;
  return 0;
}
  
//------------------------------------------------------------------
// POST MESSAGE TO SOSSRV
int sossrv_post_msg(sos_pid_t did, sos_pid_t sid, uint8_t type,
		    uint8_t length, void *data, uint16_t saddr, uint16_t daddr)
{
  SOS_Message_t txsosmsg;
  int numtxbytes;
  if (connected != 1) return -1;
  txsosmsg.did = did;
  txsosmsg.sid = sid;
  txsosmsg.daddr = ehtons(daddr);
  txsosmsg.saddr = ehtons(saddr);
  txsosmsg.type = type;
  txsosmsg.len = length;
  memcpy(txsosmsg.data, data, length);
  numtxbytes = writen(clientsockfd, (void*)&txsosmsg, 
		      SOS_MSG_HEADER_SIZE + length);
  if (numtxbytes < 0)
    exit(EXIT_FAILURE);
  return 0;
}
//----------------------------------------------------------------
// DISCONNECT
void sossrv_disconnect()
{
  close(clientsockfd);
  return;
}
//------------------------------------------------------------------
// SETUP MESSAGE HANDLER
int sossrv_recv_msg(recv_msg_func_t func)
{
  if (connected != 1) return -1;
  printf("Connected to sossrv. Creating a recv thread ...\n");
  if (pthread_create(&sossrv_rx_thread, NULL,
		     sossrv_rx_thread_handler, NULL) != 0){
    printf("Pthread Create Failed \n");
    perror("pthread_create");
    close(clientsockfd);
    exit(EXIT_FAILURE);
  }
  printf("Pthread Created \n");
  sossrv_rx_msg_handler = func;
  return 0;
}

//----------------------------------------------------------------
// RECEIVE THREAD
void *sossrv_rx_thread_handler(void *arg)
{
  Message* rxsosmsg;
  int numrxbytes;
  uint16_t uTemp;
  while (1){
    rxsosmsg = (Message*) malloc(sizeof(Message));
    if (rxsosmsg == NULL) exit(EXIT_FAILURE);
    numrxbytes = readn(clientsockfd, rxsosmsg, SOS_MSG_HEADER_SIZE);
    if ((numrxbytes < 0) || (numrxbytes < SOS_MSG_HEADER_SIZE)){
      printf("Error during read\n");
      exit(EXIT_FAILURE);
    }
    rxsosmsg->data = (uint8_t*) malloc(rxsosmsg->len);
    if (rxsosmsg->data == NULL) exit(EXIT_FAILURE);
    numrxbytes = readn(clientsockfd, rxsosmsg->data, rxsosmsg->len);
    if ((numrxbytes < 0) || (numrxbytes < rxsosmsg->len)){
      printf("Error during read\n");
      exit(EXIT_FAILURE);
    }
    // Fix Endian Ness
    uTemp = rxsosmsg->daddr;
    rxsosmsg->daddr = entohs(uTemp);
    uTemp = rxsosmsg->saddr;
    rxsosmsg->saddr = entohs(uTemp); 
    sossrv_rx_msg_handler(rxsosmsg);
    free(rxsosmsg->data);
    free(rxsosmsg);
  }
}

