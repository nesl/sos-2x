/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief radio implementation for gateway
 *
 * Currently it is empty.  Eventually, we will implement
 * socket for communication between gateways
 */
#include <hardware.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <message_queue.h>
#include <net_stack.h>
#include <sos_info.h>

#define NUM_SENDDONES_MSG        512

//! variables for sossrv
extern char *sossrv_addr;
extern int sossrv_port;
static int sock_to_sossrv;

//! variables for concurrency
static bool bTsEnable = false;
#define SEND_BUF_SIZE  576

static bool      msg_succ[NUM_SENDDONES_MSG];
static Message * senddone_buf[NUM_SENDDONES_MSG];
static int num_senddones = 0;
static bool senddone_callback_requested = false;

//static int readn(int fd, void *vptr, int nbytes);
static int writen(int fd, void *vptr, int nbytes);

static void handle_senddone( void ){
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
	Message *txmsgptr;    //!< pointer to transmit buffer
	bool succd = false;
	HAS_CRITICAL_SECTION;

	//! change ownership
	if(flag_msg_release(m->flag)){
		ker_change_own(m->data, RADIO_PID);
	}
	txmsgptr = m;

	ENTER_CRITICAL_SECTION();
	if(writen(sock_to_sossrv, (uint8_t*)txmsgptr, SOS_MSG_HEADER_SIZE) ==
			SOS_MSG_HEADER_SIZE) {
		if(writen(sock_to_sossrv, txmsgptr->data, txmsgptr->len) ==
				txmsgptr->len) {
			succd = true;
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
	msg_succ[num_senddones] = succd;
	num_senddones++;
	if( senddone_callback_requested == false ) {
		interrupt_add_callbacks(handle_senddone);
		senddone_callback_requested = true;
	}

	LEAVE_CRITICAL_SECTION();
}

void read_dummy(int len)
{
	uint8_t tmp[256];
	int cnt;
	cnt = read(sock_to_sossrv, (void*)tmp, len);

}

static void recv_thread(int fd)
{
	//! receiving header
	while(1) {
		struct Message recv_hdr;
		//uint8_t read_buf[SEND_BUF_SIZE];
		struct Message *recv_msg = NULL;
		int cnt;

		cnt = read(sock_to_sossrv, (void*)&recv_hdr, SOS_MSG_HEADER_SIZE);

		if( cnt == 0 || cnt == -1 ) {
			return;
		}

		recv_msg = msg_create();

		if( recv_msg ) {
			memcpy(recv_msg, &recv_hdr, SOS_MSG_HEADER_SIZE);
			if(recv_hdr.len != 0) {
				recv_msg->data = ker_malloc(recv_hdr.len, RADIO_PID);
				if(recv_msg->data != NULL) {
					recv_msg->flag = SOS_MSG_RELEASE;
					if(read(sock_to_sossrv, (void*)&(recv_msg->data[0]), recv_hdr.len) != recv_hdr.len) {
						msg_dispose(recv_msg);
						return;
					}
					if(bTsEnable) {
						timestamp_incoming(recv_msg, ker_systime32());
					}
					handle_incoming_msg(recv_msg, SOS_MSG_RADIO_IO);
				} else {
					msg_dispose(recv_msg);
					read_dummy(recv_hdr.len);
				}
			} else {
				recv_msg->data = NULL;
				recv_msg->flag = 0;
				if(bTsEnable) {
					timestamp_incoming(recv_msg, ker_systime32());
				}
				handle_incoming_msg(recv_msg, SOS_MSG_RADIO_IO);
			}
		} else {
			 read_dummy(recv_hdr.len);
		}
	}
}

void radio_init()
{
	struct sockaddr_in server_address;

	// Create the socket
	if ((sock_to_sossrv = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("radio.c: socket");
		exit(1);
	}

	// Setup the internet address
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(sossrv_port);
	inet_aton(sossrv_addr, &(server_address.sin_addr));
	memset(&(server_address.sin_zero), '\0', 8);

	DEBUG("Connecting to server at %s\n", inet_ntoa(server_address.sin_addr));
	DEBUG("Connecting to port at %d\n", ntohs(server_address.sin_port));

	// Connect
	if (connect(sock_to_sossrv, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1) {
		perror("radio.c: connect");
		exit(1);
	}
	DEBUG("Established TCP connection to SOSSRV..\n");


	interrupt_add_read_fd(sock_to_sossrv, recv_thread);

}

void radio_final()
{
	close(sock_to_sossrv);
}

int8_t radio_set_timestamp(bool on)
{
	bTsEnable = on;
	return SOS_OK;
}

//------------------------------------------------------------
// READ N BYTES - BLOCKING CALL
#if 0
static int readn(int fd, void *vptr, int nbytes)
{
	void* ptr;
	int nleft, nread;

	ptr = vptr;
	nread = 0;
	nleft = nbytes;

	while (nleft > 0){
		nread = read(fd, ptr, nleft);
		if (nread == -1) {
			nread = 0;
		} else if (nread < 0){
			if (errno == EINTR)
				nread = 0;
			else{
				//perror("readn");
				return -1;
			}
		}
		else if (nread == 0) {
			break;
		}

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes-nleft);
}
#endif

//------------------------------------------------------------
// WRITE N BYTES - BLOCKING CALL
static int writen(int fd, void *vptr, int nbytes)
{
	void* ptr;
	int nleft, nwrite;
	ptr = vptr;
	nleft = nbytes;
	while (nleft > 0){
		nwrite = write(fd, ptr, nleft);
		if (nwrite < 0){
			if (errno == EINTR)
				nwrite = 0;
			else{
				perror("writen");
				return -1;
			}
		}
		nleft -= nwrite;
		ptr += nwrite;
	}
	return nbytes;
}


