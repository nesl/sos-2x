/**
 * \file tree_routing_test.h
 * \brief Header file for tree routing unit test
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef TREE_ROUTING_TEST_H_
#define TREE_ROUTING_TEST_H_

#define TR_TEST_PID DFLT_APP_ID1

//-------------------------------------------------------------
// TR TEST PACKET
//-------------------------------------------------------------
typedef struct {
  uint32_t seq_no;
  uint16_t node_addr;
} PACK_STRUCT tr_test_pkt_t;

#endif//TREE_ROUTING_TEST_H_

