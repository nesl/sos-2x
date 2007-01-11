/**
 * \file tree_routing_test_pc.c
 * \brief PC app. to read the tree routing test packets
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <inttypes.h>

#include <sossrv_client.h>
#include <sossrv.h>
#include <sos_endian.h>

#include <routing/tree_routing/tree_routing.h>
#include <unit_test/modules/routing/tree_routing/tree_routing_test.h>

//------------------------------------------------------
// TYPEDEFS
typedef struct node_stats_str {
  struct node_stats_str* next;
  uint16_t node_addr;
  int numrx;
  int nummiss;
  int prevseqno;
} node_stats_t;


//------------------------------------------------------
// STATIC FUNCTIONS
static void tr_test(char* networkport);
static void printusage();
int tr_test_rx_msg(Message* rxsosmsg);
static void printtrmsg(Message* psosmsg);
static void fix_endian(Message* psosmsg);
static void updatestats(Message* psosmsg);
static node_stats_t* find_node_stat(node_stats_t* netstats, uint16_t node_addr);
static node_stats_t* insert_node_stat(node_stats_t* netstats, uint16_t node_addr);

//------------------------------------------------------
int main(int argc, char **argv)
{
  char* networkport;
  int ch;  
  char DEFAULT_SOS_PORT[] = "127.0.0.1:7915";
  networkport = DEFAULT_SOS_PORT;
  while ((ch = getopt(argc, argv, "hn:")) != -1){
    switch(ch) {
    case 'n': networkport = optarg; break;
    case '?': case 'h':
      printusage();
      exit(EXIT_FAILURE);
      break;
    }
  };
  tr_test(networkport);
  return 0;  
}
//------------------------------------------------------
static void tr_test(char* networkport)
{
  char *sossrvipaddr;
  char *sossrvportnum;

  // Parse the IP Address and Port
  sossrvipaddr = (char*)strsep(&networkport,":");
  sossrvportnum = networkport;
  


  if (sossrv_connect(sossrvipaddr, sossrvportnum) != 0){
    fprintf(stderr, "surge_stats: sossrv_connect - Failed to connect to SOS Server.\n");
    exit(EXIT_FAILURE);
  }

  sossrv_recv_msg(tr_test_rx_msg);

  while(1);
  return;
  
}
//---------------------------------------------------------------------------------
int tr_test_rx_msg(Message* rxsosmsg)
{
  fix_endian(rxsosmsg);
  printtrmsg(rxsosmsg);
  updatestats(rxsosmsg);
  return 0;
}
//---------------------------------------------------------------------------------
static void printtrmsg(Message* psosmsg)
{
  tr_test_pkt_t* testpkt;
  tr_hdr_t* tr_header;
  if ((psosmsg->did == TR_TEST_PID) && (psosmsg->type == MSG_TR_DATA_PKT)){
    tr_header = (tr_hdr_t*)(psosmsg->data);
    testpkt = (tr_test_pkt_t*)(psosmsg->data + sizeof(tr_hdr_t));
    printf("<Src: %d> <Seq No: %d> <Hops: %d> <Parent: %d>\n",
	   testpkt->node_addr, testpkt->seq_no, tr_header->originhopcount,
	   tr_header->parentaddr);
  }
  return;
}
//---------------------------------------------------------------------------------
static void fix_endian(Message* psosmsg)
{
  tr_test_pkt_t* testpkt;
  tr_hdr_t* tr_header;
  if ((psosmsg->did == TR_TEST_PID) && (psosmsg->type == MSG_TR_DATA_PKT)){
    tr_header = (tr_hdr_t*)(psosmsg->data);
    tr_header->originaddr = ehtons(tr_header->originaddr);
    tr_header->seqno = ehtons(tr_header->seqno);
    tr_header->parentaddr = ehtons(tr_header->parentaddr);
    testpkt = (tr_test_pkt_t*)(psosmsg->data + sizeof(tr_hdr_t));
    testpkt->seq_no = ehtonl(testpkt->seq_no);
    testpkt->node_addr = ehtons(testpkt->node_addr);
  }
  return;
}
//---------------------------------------------------------------------------------
static void updatestats(Message* psosmsg)
{
  tr_test_pkt_t* testpkt;
  tr_hdr_t* tr_header;
  node_stats_t* node_stats;
  static node_stats_t* netstats = NULL;
  
  if ((psosmsg->did == TR_TEST_PID) && (psosmsg->type == MSG_TR_DATA_PKT)){
    tr_header = (tr_hdr_t*)(psosmsg->data);
    testpkt = (tr_test_pkt_t*)(psosmsg->data + sizeof(tr_hdr_t));
    
    if ((node_stats = find_node_stat(netstats, testpkt->node_addr)) == NULL){
      netstats = insert_node_stat(netstats, testpkt->node_addr);
      node_stats = find_node_stat(netstats, testpkt->node_addr);
    }

    // Update no. of rx packets
    node_stats->numrx++;
    // Update no. of missed pkts.
    if ((testpkt->seq_no > node_stats->prevseqno) && (node_stats->prevseqno != -1))
      node_stats->nummiss += (testpkt->seq_no - node_stats->prevseqno - 1);
    // Update Prev. Seq. No.
    node_stats->prevseqno = testpkt->seq_no;
    // Print Stats
    printf("<ID Rx Miss>: ");
    for (node_stats = netstats; node_stats != NULL; node_stats = node_stats->next)
      {
	printf("<%d %d %d> ", node_stats->node_addr, node_stats->numrx, node_stats->nummiss);
      }
    printf("\n");
  }
   return;
}
//---------------------------------------------------------------------------------
static node_stats_t* find_node_stat(node_stats_t* netstats, uint16_t node_addr)
{
  node_stats_t* n;
  for (n = netstats; n != NULL; n = n->next){
    if (n->node_addr == node_addr){
      return n;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------------
static node_stats_t* insert_node_stat(node_stats_t* netstats, uint16_t node_addr){
  node_stats_t* n;
  if ((n = malloc(sizeof(node_stats_t))) == NULL){
    fprintf(stderr, "insert_node_stat: Out of memory.\n");
  }
  n->next = netstats;
  n->node_addr = node_addr;
  n->nummiss = 0;
  n->prevseqno = -1;
  n->numrx = 0;
  return n;
}
//---------------------------------------------------------------------------------
// ERROR EXIT FUNCTION
static void printusage()
{
  printf("tree_routing_test_pc [-n <TCP Port>]\n");
  printf(" -n <TCP Port>: TCP Port is <IP Addr:Port Num> e.g. 127.0.0.1:7915\n");
}
