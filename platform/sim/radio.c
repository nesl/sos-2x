/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * @brief radio emulation
 * 9/15/2005 mods made by Nicholas Kottenstette (nkottens@nd.edu)
 *     changed so that each mote that is simulated uses its
 *     unique kernel information as if it were on an actual mote
 *     this allows simulation of the gatway mote for loading and
 *     unloading modules!
 */
#include <hardware.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sched.h>
#include <message_queue.h>
#include <net_stack.h>
#include <sos_info.h>

#include "radio.h"

#ifndef SOS_DEBUG_RADIO
#undef DEBUG
#define DEBUG(...)
#endif


#define MAXLENHOSTNAME	   256
#define LINE_BUF_SIZE      255
#define TOPO_TYPE_SELF     1
#define TOPO_TYPE_NEIGHBOR 2
#define TOPO_TYPE_OTHER    3
#define NUM_SENDDONES_MSG        512

//! this is defined in hardware.c
extern char *topofile;
extern uint16_t radio_pkt_success_rate;

typedef struct Topology{
	int id;
	int sock; // file descriptor for that node
    int unit;
	int x;
	int y;
	int z;
    unsigned int r2;
	int type;
	double succ_rate;
} Topology;

static Topology topo_self;
static Topology *topo_array;
static int totalNodes = 0;             // total number of nodes in topofile
static struct sockaddr_in	sockaddr;

/**
 * @brief sender state
 */
//! priority queue
static bool bTsEnable = false;

static int send_sock;

static int getj(int16_t id);
static void print_nodes();
static int get_sin_port(int16_t id);

static bool      msg_succ[NUM_SENDDONES_MSG];
static Message * senddone_buf[NUM_SENDDONES_MSG];
static int num_senddones = 0;
static bool senddone_callback_requested = false;

/**
 * @brief receiver state
 */
#define SEND_BUF_SIZE  576


static void handle_senddone( void )
{
	int i;
	for( i = 0; i < num_senddones; i++ ) {
		msg_send_senddone(senddone_buf[i], msg_succ[i], RADIO_PID);
	}

	num_senddones = 0;
	senddone_callback_requested = false;
}

/**
 * @brief allocate send buffer
 */
void radio_msg_alloc(Message *m)
{
	int sock = send_sock;
	struct sockaddr_in name;
	Message *txmsgptr = m;    //!< pointer to transmit buffer
	HAS_CRITICAL_SECTION;

	//! change ownership
	if(flag_msg_release(m->flag)){
		ker_change_own(m->data, RADIO_PID);
	}
	DEBUG("Radio: Got packet to send\n");

	ENTER_CRITICAL_SECTION();

	if(txmsgptr->type == MSG_TIMESTAMP)
	{ 
		uint32_t timestamp = ker_systime32();
		memcpy(txmsgptr->data, (uint8_t*)(&timestamp),sizeof(uint32_t));  
	}


	name.sin_family = AF_INET;
	name.sin_addr.s_addr = sockaddr.sin_addr.s_addr;

	//! packet comes in, send all of them
	{
		int i = 0;
		int bytes_sent;
		bool succ = false;
		/* See man rand for why this way is prefered */
		//r = (int) (100.0*rand()/RAND_MAX);
		// always broadcast
		uint8_t send_buf[SEND_BUF_SIZE];
		int k = 0;
		DEBUG("sim: send_thread %d\n",radio_pkt_success_rate);
		for(i = 0; i < SOS_MSG_HEADER_SIZE; i++, k++) {
			send_buf[k] = *(((uint8_t*)txmsgptr) + i);
		}
		for(i = 0; i < txmsgptr->len; i++, k++) {
			send_buf[k] = txmsgptr->data[i];
		}

		DEBUG("totalNodes = %d\n", totalNodes);
		for(i = 0; i < totalNodes; i++){
			if(topo_array[i].type == TOPO_TYPE_NEIGHBOR){
				int r;
				r = (int) (100.0*rand()/RAND_MAX);
				if(r <= radio_pkt_success_rate){
					if((txmsgptr->daddr == topo_array[i].id) ||
							(txmsgptr->daddr == BCAST_ADDRESS))
						succ = true;
					name.sin_port = htons( get_sin_port(topo_array[i].id) );
					DEBUG("sim: sending to topo_array[%d].id = %d\n",i,topo_array[i].id);
					bytes_sent = sendto(sock, send_buf, k, 0,
							(struct sockaddr *)&name,
							sizeof(struct sockaddr_in));
					if (bytes_sent < 0) {
						DEBUG("Radio: Error Sending Packet ! \n");
					}
					else {
						DEBUG("Radio: Sent %d bytes to port %d\n", bytes_sent, ntohs(name.sin_port));
					}
				}
			}
		}
		if(bTsEnable) {
			timestamp_outgoing(txmsgptr, ker_systime32());
		}

		if( num_senddones == NUM_SENDDONES_MSG ) {
			fprintf(stderr, "no enough buffer for holding SENDDONE\n");
			fprintf(stderr, "increase NUM_SENDDONES_MSG in platform/sim/radio.c\n");
			exit( 1 );
		}

		senddone_buf[num_senddones] = txmsgptr;
		msg_succ[num_senddones] = succ;
		num_senddones++;
		if( senddone_callback_requested == false ) {
			interrupt_add_callbacks(handle_senddone);
			senddone_callback_requested = true;
		}
	}
	LEAVE_CRITICAL_SECTION();
}

static void print_nodes()
{
	int myj;
	fprintf(stderr, "ker_id() : %d\n", ker_id());
	fprintf(stderr, "radio_pkt_success_rate: %d\n",radio_pkt_success_rate);
	fprintf(stderr, "totalNodes = %d\n", totalNodes);
	for(myj=0;myj < totalNodes; myj++){
		fprintf(stderr, "topo_array[%d].id = %d\n",myj,topo_array[myj].id);
		fprintf(stderr, "topo_array[%d].sock = %d\n",myj,topo_array[myj].sock);
		fprintf(stderr, "topo_array[%d].unit = %d\n",myj,topo_array[myj].unit);
		fprintf(stderr, "topo_array[%d].x = %d\n",myj,topo_array[myj].x);
		fprintf(stderr, "topo_array[%d].y = %d\n",myj,topo_array[myj].y);
		fprintf(stderr, "topo_array[%d].z = %d\n",myj,topo_array[myj].z);
		fprintf(stderr, "topo_array[%d].r2 = %d\n",myj,topo_array[myj].r2);
		fprintf(stderr, "topo_array[%d].type = %u\n",myj,topo_array[myj].type);
	}
}

static int getj(int16_t id)
{
	int myj;
	for(myj = 0; myj < totalNodes; myj++){
		if(topo_array[myj].id == id)
			break;
	}
	if( myj == totalNodes ){
		fprintf(stderr, "myj is not found\n");
		fprintf(stderr, "nid = %d\n", id);
		print_nodes();
		exit(1);
	}
	return myj;
}
/**
 * @brief receiver thread
 */
static void recv_thread(int fd)
{
	DEBUG("radio: receiver start\n");
	while(1)
	{
		uint8_t read_buf[SEND_BUF_SIZE];
		Message *recv_msg = NULL;
		int cnt;

		//sched_yield();
		DEBUG("Radio: Receiver Idle ... \n");
		cnt = read(topo_self.sock, read_buf, SEND_BUF_SIZE);
		// XXX Using recvfrom will cause problem on Cygwin
		//cnt = recvfrom(topo_self.sock, read_buf, SEND_BUF_SIZE, 0, &fromaddr, &fromaddrlen);
		DEBUG("Radio: Read %d bytes\n", cnt);
		if(cnt < 0 && cnt != -1) {
			DEBUG("Radio: unable to read, exiting...\n");
			exit(1);
		}
		if( cnt == 0 || cnt == -1) {
			return;
		}
		if(cnt < SOS_MSG_HEADER_SIZE) {
			DEBUG("Radio: get incomplete header\n");
			//! something is wrong...
			return;
		}
		if((((Message*)read_buf)->len) != (cnt - SOS_MSG_HEADER_SIZE)) {
			DEBUG("Radio: invalid data payload size\n");
			return;
		}
		recv_msg = msg_create();
		if(recv_msg == NULL) {
			DEBUG("Radio: no message header\n");
			return;
		}
		memcpy(recv_msg, read_buf, SOS_MSG_HEADER_SIZE);

		if(recv_msg->len != 0) {
			recv_msg->data = ker_malloc(recv_msg->len, RADIO_PID);
			if(recv_msg->data != NULL) {
				recv_msg->flag = SOS_MSG_RELEASE;
				memcpy(recv_msg->data, read_buf + SOS_MSG_HEADER_SIZE, recv_msg->len);
				if(recv_msg->type == MSG_TIMESTAMP)
				{
					uint32_t timestamp = ker_systime32();
					memcpy(&recv_msg->data[4], (uint8_t *)(&timestamp), sizeof(uint32_t));
				}

				if(bTsEnable) {
					timestamp_incoming(recv_msg, ker_systime32());
				}
				DEBUG("handle incoming msg \n");
				handle_incoming_msg(recv_msg, SOS_MSG_RADIO_IO);
			} else {
				msg_dispose(recv_msg);
			}
		} else {
			recv_msg->data = NULL;
			recv_msg->flag = 0;
			if(bTsEnable) {
				timestamp_incoming(recv_msg, ker_systime32());
			}
			handle_incoming_msg(recv_msg, SOS_MSG_RADIO_IO);
		}
	}
}

static int get_sin_port(int16_t id)
{
	return (int) (SIM_PORT_OFFSET+SIM_MAX_GROUP_ID+1)
		+ node_group_id*(SIM_MAX_MOTE_ID+1) + id;
}
void sim_radio_init()
{
	//! initialize node id and location
	FILE *fid;
	char line_buf[LINE_BUF_SIZE];    // line buffer
	int j = 0;

	if((fid = fopen(topofile, "r")) == NULL){
		char* sosrootdir;
		char newtopofile[256];
		sosrootdir = getenv("SOSROOT");
		strcpy(newtopofile, sosrootdir);
		printf("Unable to open %s\n", topofile);
		strcat(newtopofile, "/tools/admin/topo.def\0");
		if((fid = fopen(newtopofile, "r")) == NULL){
			printf("Unable to open %s\n", newtopofile);
			exit(1);
		}
		else
			topofile = newtopofile;
	}
	printf("Using topology file %s\n", topofile);
	// remove comments
	do{
		fgets(line_buf, LINE_BUF_SIZE, fid);
	} while(line_buf[0] == '#');

	if(sscanf(line_buf, "%d", &totalNodes) != 1){
		fprintf(stderr, "no data in %s\n", topofile);
		exit(1);
	}

	topo_array = (Topology*)malloc((totalNodes + 1) * sizeof(Topology));
	if (topo_array == NULL){
		fprintf(stderr, "not enough memory\n");
		exit(1);
	}

	for(j = 0; j < totalNodes; j++){
		do{
			// remove comments
			fgets(line_buf, LINE_BUF_SIZE, fid);
		}while(line_buf[0] == '#');
		if(sscanf(line_buf, "%d %d %d %d %d %u",
					&topo_array[j].id,
					&topo_array[j].unit,
					&topo_array[j].x,
					&topo_array[j].y,
					&topo_array[j].z,
					&topo_array[j].r2) != 6){
			fprintf(stderr, "not enough definitions in %s: %s\n", topofile, line_buf);
			exit(1);
		}
		//topo_array[j].id = j;
		topo_array[j].type = TOPO_TYPE_OTHER;
		topo_array[j].sock = -1;
	}
#if 0
	print_nodes();
	exit(1);
#endif
	if(fclose(fid) != 0){
		perror("fclose");
		exit(1);
	}

	// finding node id by finding available port
	//for(j = 1; j <= totalNodes; j++)
	{
		int sock;
		int myj = getj(ker_id());
		struct sockaddr_in name;
		sock = socket(AF_INET, SOCK_DGRAM, 0);

		if (sock < 0) {
			perror("opening datagram socket");
			exit(1);
		}

		/* Create name with wildcards. */
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = INADDR_ANY;
		//		name.sin_port = htons(20000 + ker_id());
		name.sin_port = htons( get_sin_port(ker_id()) );

		if (bind(sock, (struct sockaddr *)&name, sizeof(name)) == 0) {
			//node_address = j;
			// successfully get an id
			topo_array[myj].sock = sock;
			topo_array[myj].type = TOPO_TYPE_SELF;
			topo_self = topo_array[myj];
			//! assign sos_info
			node_loc.x = topo_self.x;
			node_loc.y = topo_self.y;
			node_loc.z = topo_self.z;
			node_loc.unit = topo_self.unit;
		}else{
			fprintf(stderr, "Unable to allocate UDP for this node!\n");
			fprintf(stderr, "Perhaps you are using the same node ID\n");
			fprintf(stderr, "Use -n <node address> to specify different address\n\n");
			exit(1);
		}
	}
	{
		uint16_t id;
		node_loc_t self;
		id = ker_id();
		self = ker_loc();
		if((id != topo_self.id) || (self.x != topo_self.x) ||
				(self.y != topo_self.y) || (self.z != topo_self.z) ||
				(self.unit != topo_self.unit)){
			fprintf(stderr, "topo file and self settings do not agree.\n");
			fprintf(stderr, "ker_id() = %d : topo_self.id = %d\n",
					id, topo_self.id);
			fprintf(stderr, "self.unit = %d : topo_self.unit = %d\n",
					self.unit, topo_self.unit);
			fprintf(stderr, "self.x = %d : topo_self.x = %d\n",
					self.x, topo_self.x);
			fprintf(stderr, "self.y = %d : topo_self.y = %d\n",
					self.y, topo_self.y);
			fprintf(stderr, "self.z = %d : topo_self.z = %d\n",
					self.z, topo_self.z);
			exit(1);
		}
	}

	for(j = 0; j < totalNodes; j++){
		uint32_t r2;
		uint16_t id;
		node_loc_t self, neighbor;
		id = ker_id();
		self = ker_loc();
		if(topo_array[j].type == TOPO_TYPE_SELF) continue;
		neighbor.unit = topo_array[j].unit;
		neighbor.x = topo_array[j].x;
		neighbor.y = topo_array[j].y;
		neighbor.z = topo_array[j].z;
		r2 = ker_loc_r2(&self,&neighbor);
		if(r2 < 0){
			fprintf(stderr, "units for neighbor do not agree.\n");
			fprintf(stderr, "self.unit = %d : neighbor.unit = %d.\n",
					self.unit, neighbor.unit);
			exit(1);
		}
		DEBUG("neighbor %d r2 = %d, self r2 = %d\n", topo_array[j].id, r2, topo_self.r2);
		if(r2 <= topo_self.r2){
			topo_array[j].type = TOPO_TYPE_NEIGHBOR;
			DEBUG("node %d is reachable\n", topo_array[j].id);
		}
	}
	{
		struct hostent	*hostptr;
		char theHost [MAXLENHOSTNAME];
		unsigned long hostaddress;

		/* find out who I am */

		if ((gethostname(theHost, MAXLENHOSTNAME))<0) {
			perror ("could not get hostname");
			exit(1);
		}
		//DEBUG("-- found host name = %s\n", theHost);

		if ((hostptr = gethostbyname (theHost)) == NULL) {
			perror ("could not get host by name, use 127.0.0.1");
			if(( hostptr = gethostbyname ("127.0.0.1") ) == NULL ) {
				perror ("Cannot get host 127.0.0.1");
				exit(1);
			}

		}
		hostaddress = *((unsigned long *) hostptr->h_addr);

		/* init the address structure */
		sockaddr.sin_family    	= AF_INET;
		sockaddr.sin_port		= htons( get_sin_port(ker_id()) );
		sockaddr.sin_addr.s_addr 	= hostaddress;
	}
	//! assign locations
	//node_loc.x = (uint16_t)(topo_self.x);
	//node_loc.y = (uint16_t)(topo_self.y);
	//print_nodes();
}

void radio_init()
{
	send_sock = socket(AF_INET, SOCK_DGRAM, 0);

	if( send_sock < 0 ) {
		perror("socket in radio.c");
		exit(1);
	}

	interrupt_add_read_fd(topo_self.sock, recv_thread);
}

void radio_final()
{
	close(topo_self.sock);
	close(send_sock);

	DEBUG("radio: shutdown\n");
}

int8_t radio_set_timestamp(bool on)
{
	bTsEnable = on;
	return SOS_OK;
}
