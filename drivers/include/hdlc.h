/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#ifndef _HDLC_H_
#define _HDLC_H_

// needed for up/down event definitions
//#include "hclp.h"

#ifndef _MODULE_
//int8_t hdlc_init();
#endif
/**
 * From RFC1662
 *
 * A summary of the PPP HDLC-like frame structure is shown below. 
 * 
 * +----------+----------+----------+
 * |   Flag   | Address  | Control  |
 * | 01111110 | 11111111 | 00000011 |
 * +----------+----------+----------+
 * +----------+-------------+---------+
 * | Protocol | Information | Padding |
 * | 8/16 bits|      *      |    *    |
 * +----------+-------------+---------+
 * +----------+----------+-----------------
 * |   FCS    |   Flag   | Inter-frame Fill
 * |16/32 bits| 01111110 | or next Address
 * +----------+----------+-----------------
 *
 * On reception, the Address and Control fields are decompressed by examining
 * the first two octets.  If they contain the values 0xff and 0x03, they are
 * assumed to be the Address and Control fields.  If not, it is assumed that
 * the fields were compressed and were not transmitted.
 * 
 */

/**
 * Each Flag Sequence, Control Escape octet, and any octet which is flagged in
 * the sending Async-Control- Character-Map (ACCM), is replaced by a two octet
 * sequence consisting of the Control Escape octet followed by the original
 * octet exclusive-or'd with hexadecimal 0x20.
 *             
 * 0x7e is encoded as 0x7d, 0x5e.    (Flag Sequence)
 * 0x7d is encoded as 0x7d, 0x5d.    (Control Escape)
 * 0x03 is encoded as 0x7d, 0x23.    (ETX)
 *
 */
#define HDLC_FLAG 0x7e
#define HDLC_CTR_ESC 0x7d
#define HDLC_EXT 0x03

/**
 * SOS protocol types
 *
 * from rfc 1662, rfc 1661 and rfc 1340
 * using address from the reserved space 0x01-0x1f
 * ppp requires that when using a private address the value must be odd
 */
#define HDLC_SOS_MSG 0x01
// 0x03 skipped because it is a escaped character
#define HDLC_RAW     0x05

#define HDLC_PROTOCOL_SIZE (sizeof(uint8_t))

// 
/*
 * all unlabled transitions are assumed to occure for any byte 0xXX
 * 
 * +------+            +-------+       +---------+       +---------+
 * |      | 0x7e       |       | 0xff  |         | 0x03  |         |
 * | IDLE |-------+--->| START |------>| ADDRESS |------>| CONTROL |
 * |      |       |    |       |       |         |       |         |
 * +------+       |    +-------+       +---------+       +---------+
 *    ^           |        |                |                 |
 *    |           |        +----------------+-----------------+
 *    |           | 0x7e   |							  |
 *    |           |        |							  |
 *    |           |        |                V
 *    |           |    +--------+         +-------+
 *    |  timeout  |    |        |   0x7e  |       |
 *    +-----------+----| ESCAPE |<--------| DATA  |
 *                     |        |         |       |
 *                     +--------+         +-------+
 *                         |                  ^
 *                         |  0x5e/0x5d/0x23  |
 *                         +------------------+
 */

/*
 * HDLC states (rx/tx agnostic)
 */
enum {
	HDLC_IDLE=0,
	HDLC_START,			// intial start byte
	HDLC_RESTART,		// start byte between back to back packets (only used by sender)
	HDLC_ADDRESS,		// address byte (not used in compressed mode)
	HDLC_CONTROL,		// control byte (not used in compressed mode)
	HDLC_PROTOCOL,	// using this field for a sequence number
	HDLC_DATA,			// packet data
	HDLC_ESCAPE,		// escape sequence
	HDLC_PADDING,		// padding if necessary (not used)
	HDLC_CRC,				// crc
};

enum {
	DUPLEX=1,
	MULTI_MASTER=(1<<1),
};

/**
 * Module state structure
 */
typedef struct {
	uint8_t pid;
	uint8_t state;
} hdlc_state_t;

#endif // _HDLC_H_
