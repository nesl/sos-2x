/**
 * \file loader_snoop.c
 * \brief Snoops for the loader packets on the air and pretty prints them
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sossrv_client.h>
#include <sos_endian.h>

#include <loader.h>
#include <sos_module_fetcher.h>

static int loader_snoop(char* snoopsrvip);
static int loader_snoop_rx_msg(Message* snoopmsg);
static void printf_fetcher_request(Message* snoopmsg);
static void printf_fetcher_fragment(Message* snoopmsg);

int main(int argc, char** argv)
{
  if (argc < 2){
    printf("Usage: \n");
    printf("loader_snoop <snoop_server_ip_addr:snoop_server_port>\n");
    exit(EXIT_FAILURE);
  }
  loader_snoop(argv[1]);
  return 0;
}

static int loader_snoop(char* snoopsrvip)
{
  char *snoopsrvipaddr, *snoopsrvport;
  
  // Parse the IP Address and Port
  snoopsrvipaddr = (char*)strsep(&snoopsrvip,":");
  snoopsrvport = snoopsrvip;
  
  if (sossrv_connect(snoopsrvipaddr, snoopsrvport) != 0){
    fprintf(stderr, "loader_snoop: sossrv_connect - Failed to connect to SOS Server.\n");
    exit(EXIT_FAILURE);
  }
  sossrv_recv_msg(loader_snoop_rx_msg);
  while(1);
  return 0;
}


static int loader_snoop_rx_msg(Message* snoopmsg)
{
  if ((snoopmsg->did == KER_DFT_LOADER_PID) || (snoopmsg->sid == KER_DFT_LOADER_PID)){
    printf("====LOADER ===========\n");
    printf("%d -----> %d\n", snoopmsg->saddr, snoopmsg->daddr);
    switch (snoopmsg->type){
    case MSG_VERSION_ADV:
      {
	msg_version_adv_t* msgadv;
	msgadv = (msg_version_adv_t*)(snoopmsg->data);
	printf("VERSION_ADV: %d\n", msgadv->version);
	break; 
      }
    case MSG_VERSION_DATA:
      {
	printf("VERSION_DATA\n");
	break;
      }
    case MSG_LOADER_LS_ON_NODE:
      {
	printf("LOADER_LS_ON_NODE\n");
	break;
      }
    case MSG_LOADER_LS_REPLY:
      {
	printf("LOADER_LS_REPLY\n");
	break;
      }
    default:
      {
	printf("UNKNOWN LOADER MESSAGE\n");
	break;
      }
    }
  }
  
  if ((snoopmsg->did == KER_FETCHER_PID) || (snoopmsg->sid == KER_FETCHER_PID)){
    printf("====FETCHER ===========\n");
    printf("%d -----> %d\n", snoopmsg->saddr, snoopmsg->daddr);
    switch (snoopmsg->type){
    case MSG_FETCHER_REQUEST:
      {
	printf("FETCHER REQUEST MESSAGE\n");
	printf_fetcher_request(snoopmsg);
	break;
      }
    case MSG_FETCHER_FRAGMENT:
      {
	printf("FETCHER FRAGMENT MESSAGE\n");
	printf_fetcher_fragment(snoopmsg);
	break;
      }
    default:
      {
	printf("UNKNOWN FETCHER MESSAGE\n");
	break;
      }
    }
  }
  return 0;
}


static void printf_fetcher_request(Message* snoopmsg)
{
  fetcher_bitmap_t* fbmp;
  int i, j;
  uint8_t mask;
  fbmp = (fetcher_bitmap_t*)(snoopmsg->data);
  printf("Key: %d ",fbmp->key);
  printf("BMP: %d bytes\n",fbmp->bitmap_size);
  printf("BMP: ");
  for (i = 0; i < fbmp->bitmap_size; i++){
    for (j = 0, mask = 1; j < 8; j++, mask <<= 1){
      if ((mask & fbmp->bitmap[i]) != 0){
	printf("X ");
      }
      else
	printf("O ");
    }
  }
  printf("\n");
  return; 
}

static void printf_fetcher_fragment(Message* snoopmsg)
{
  fetcher_fragment_t* ffrag;
  ffrag = (fetcher_fragment_t*)snoopmsg->data;
  printf("Key: %d FragID: %d\n", ffrag->key, ffrag->frag_id);
  return;
}
