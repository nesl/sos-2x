/**
 * @file surge_dump.c
 * @breif dumps surge messages into a file. A catcher for sos_dump.exe.
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include <stdio.h>

#include "sos_catch.h"

#include <message_types.h>
#include <tree_routing.h>
#include <surge.h>

// Where to connect to the sos server:
const char *ADDRESS="127.0.0.1";
const char *PORT="7915";

/*
 * Catches surge messages and dumps them into surge.dat.
 */
int catch(Message* msg) {
  int i; // Loop variable.
  char tree_routing_header[sizeof(tr_hdr_t)];
  char surge_message[sizeof(SurgeMsg)];
  FILE *fout; // The file we'll be writing to.
  const char* FILENAME = "surge.dat";

  // We only care about tree routing messages:
  if (msg->type == MSG_TR_DATA_PKT) {
    // Split off the tree routing header:
    for(i=0; i < sizeof(tr_hdr_t); ++i) {
      tree_routing_header[i] = msg->data[i];
    }
    tr_hdr_t* tr_hdr = (tr_hdr_t*)(tree_routing_header);

    // We only care about messages for the Surge module:
    if (tr_hdr->dst_pid == SURGE_MOD_PID) {
      // Get the surge message:
      for(i=0; i < sizeof(SurgeMsg); ++i) {
        surge_message[i] = msg->data[sizeof(tr_hdr_t)+i];
      }
      SurgeMsg* sg_msg = (SurgeMsg*)(surge_message);

      // Flip the endiannesses of the relevant fields:
      tr_hdr->originaddr = entohs(tr_hdr->originaddr);
      tr_hdr->seqno = entohs(tr_hdr->seqno);
      tr_hdr->parentaddr = entohs(tr_hdr->parentaddr);
      sg_msg->originaddr = entohs(sg_msg->originaddr);
      sg_msg->reading = entohs(sg_msg->reading);
      sg_msg->seq_no = entohl(sg_msg->seq_no);

      // Sanity checking:
      if (tr_hdr->originaddr != sg_msg->originaddr) {
        printf("Tragic loss of coherence: origin addresses don't agree:\n");
        printf("Tree Routing: %d\nSurge: %d\n", tr_hdr->originaddr,
               sg_msg->originaddr);
        exit(1);
      }

      // Open the file:
      fout = fopen(FILENAME,"a+");

      // Print the relevant message components.
      // The surge origin address is dropped because it's the same as the
      // tree routing origin address.
      fprintf(fout, "origin_address:%d\t", tr_hdr->originaddr);
      fprintf(fout, "routing_sequence_number:%d\t", tr_hdr->seqno);
      fprintf(fout, "hop_count:%d\t", tr_hdr->hopcount);
      fprintf(fout, "origin_hop_count:%d\t", tr_hdr->originhopcount);
      fprintf(fout, "parent_address:%d\t", tr_hdr->parentaddr);
      fprintf(fout, "surge_message_type:%d\t", sg_msg->type);
      fprintf(fout, "surge_sequence_number:%d\t", sg_msg->seq_no);
      fprintf(fout, "reading:%d\n", sg_msg->reading);

      fclose(fout);
    }
  }
  // There are no else cases: messages which aren't Surge messages just go
  // right on by without being touched. If you want to be able to see what
  // else is going on, uncomment the following lines (and comment the line
  // above):
/*
    else {
      printf("Message destination (%d) is not the Surge module (%d).\n",
             tr_hdr->dst_pid, SURGE_MOD_PID);
    }
  }
  else {
    printf("Message (type %d) is not a tree routing message (type %d)\n",
           msg->type, MSG_TR_DATA_PKT);
  }
*/
}

// Subscribe the catch function and let it do it's thing.
int main(int argc, char **argv)
{
  int ret; // Did subscription succeed?

  ret = sos_subscribe(ADDRESS, PORT, (recv_msg_func_t)catch);

  if (ret) {
    return ret;
  }

  // Let things run:
  while (1){
    sleep(1);
  }

  return 0;
}
