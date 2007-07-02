/**
 * @file catch_surge.c
 * @breif handles tree routing messages enclosing surge messages
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include <stdio.h>

/*
 * The relevant message structures are defined here:
 */
#include <message_types.h>
#include <tree_routing.h>
#include <surge.h>

/*
 * This function is set up to recieve sos messages by sos_dump.c
 * Write whatever custom message handler you want here.
 */
int catch(Message* msg) {
  int i; // loop variable

  if (msg->type != MSG_TR_DATA_PKT) {
    printf("--------------------------------\n");
    printf("Dest Mod Id: %d\n", msg->did);
    printf("Src  Mod Id: %d\n", msg->sid);
    printf("Dest Addr  : %X\n", msg->daddr);
    printf("Src  Addr  : %X\n", msg->saddr);
    printf("Msg Type   : %d\n", msg->type);
    printf("Msg Length : %d\n", msg->len);
  } else {
    // first, parse the data into the tree routing header and surge message:
    char tree_routing_header[sizeof(tr_hdr_t)];
    char surge_message[sizeof(SurgeMsg)];
    for(i=0; i < sizeof(tr_hdr_t); ++i) {
      tree_routing_header[i] = msg->data[i];
    }
    for(i=0; i < sizeof(SurgeMsg); ++i) {
      surge_message[i] = msg->data[sizeof(tr_hdr_t)+i];
    }

    // cast the pieces to pointers:
    tr_hdr_t* tr_hdr = (tr_hdr_t*)(tree_routing_header);
    SurgeMsg* sg_msg = (SurgeMsg*)(surge_message);

    // flip the endiannesses of the relevant fields:
    tr_hdr->originaddr = entohs(tr_hdr->originaddr);
    tr_hdr->seqno = entohs(tr_hdr->seqno);
    tr_hdr->parentaddr = entohs(tr_hdr->parentaddr);

    // print the tree routing header:
    printf("Origin address: %d\n", tr_hdr->originaddr);
    printf("TR Sequence number: %d\n", tr_hdr->seqno);
    printf("Hop count: %d\n", tr_hdr->hopcount);
    printf("Origin hop count: %d\n", tr_hdr->originhopcount);
    printf("Destination Module: %X (==%d)\n", tr_hdr->dst_pid, tr_hdr->dst_pid);
    printf("Parent address: %d\n", tr_hdr->parentaddr);

    // print the surge message:
    printf("------------------\n");
    printf("Surge message type: %d\n", sg_msg->type);
    if (sg_msg->type == 7) { //LAME HACK = ILLITERATE
      printf("Mote working, MDA dysfunctional\n\n");
      return 0;
    }

    // print the reading:
    if (tr_hdr->originaddr != 1) {
      sg_msg->reading = entohs(sg_msg->reading);
      sg_msg->originaddr = entohs(sg_msg->originaddr);
      sg_msg->seq_no = entohl(sg_msg->seq_no);

      printf("Reading: %d\n", sg_msg->reading);
    }
    
    printf("Origin address: %d\n", sg_msg->originaddr);
    printf("PS Sequence number: %d\n", sg_msg->seq_no);
    
    printf("Message Data: \n");
    for (i = 0; i < msg->len; ++i) {
      if ((i%16) == 0) printf("\n");
      printf("%X ", msg->data[i]);
    }
  }

  printf("   CRC: %X %X\n", msg->data[msg->len], msg->data[msg->len+1]);
  printf("\n");
}
