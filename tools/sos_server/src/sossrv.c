/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * \file sossrv.c
 * \brief The gateway application for the Sossrv middleware
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
/**
 * Endian-ness Assumptions:
 * The clients connecting to the sossrv are of the same endian-ness as the machine running sossrv.
 * The clients take care of the endian-ness issues SOS messages.
 */

//-----------------------------------------------------------------------------
// INCLUDES
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dev_network.h>     // For the TCP connection setup
#include <dev_serial.h>      // For the serial port setup
#include <parsecmd.h>        // For command line parsing
#include <sossrv.h>          // Default value definitions and data types
#include <sock_utils.h>      // Simple socket utils
#include <hdlc.h>
// this needs to be consistant with what is in sos_info.h
//#define UART_MAX_MSG_LEN 0x80

//-----------------------------------------
// MACROS AND DEFINITIONS
#define LISTENER_BACKLOG 10
#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))
#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))

//-----------------------------------------
// LOCAL FUNCTION PROTOTYPES
static int printsosmsg(SOS_Message_t* psosmsg, unsigned short crc, char *header);
static int printrawmsg(unsigned char *buff, int len, char *header);
static int serial_pkt_handler();
static int network_pkt_handler();
static int dispatch_sos_message(SOS_Message_t* psosmsg);
static unsigned short computeMsgCRC(unsigned char protocol, SOS_Message_t* psosmsg);
static unsigned short crcByte(unsigned short crc, unsigned char b);


//-----------------------------------------
// VARIABLE DECLARATIONS
int listenerfd;                 //! Listening socket descriptor of the server
int server_port;                //! Port Numer of the server
struct sockaddr_in server_addr; //! Server address

char* serial_device;            //! Serial device to connect to the SOS NIC
int serialfd = -1;              //! The socket descriptor of the serial device
int serial_baudrate;            //! Baudrate of the serial device

char* network_port;             //! TCP connection to ethernet if any

fd_set master_fds;              //! Master file descriptor list
fd_set read_fds;                //! Temp file descriptor list for select()  
int fdmax;                      //! Maximum file descriptor number
int curr_sock_fd;               //! The current socket descriptor

int newfd;                      //! Newly accept()ed socket descriptor
struct sockaddr_in remoteaddr;  //! Client address
int addrlen;                    //! 

int outputOpts = OUTPUT_DEFAULT;

void sig_handler(int sig)
{
	switch(sig){
		case SIGTERM: case SIGINT:
			if( serialfd != -1) {
				close(serialfd);
			}
			exit(1);
			break;
		default:
			break;
	}
}


//-----------------------------------------------------------------------------
// MAIN
int main(int argc, char *argv[])
{
  int yes = 1;                      //! For setsockopt() SO_REUSEADDR

	if(signal(SIGTERM, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGTERM failed\n");
		exit(1);
	}

	if(signal(SIGINT, sig_handler) == SIG_ERR){
		fprintf(stderr, "ignore SIGINT failed\n");
		exit(1);
	}

  //--------------------------------------------------------------------------
  // INITIALIZE THE VARIABLES
  server_port = DEFAULT_SERVER_PORT;
  serial_device = DEFAULT_SERIAL_PORT;
  serial_baudrate = DEFAULT_BAUDRATE;
  network_port = NULL;
  parsecmdline(argc, argv, &server_port, &serial_device, &serial_baudrate, &network_port, &outputOpts);
  printf("SOSSRV PARAMETERS:\n");
    
  //--------------------------------------------------------------------------
  // SETUP SERIAL CONNECTION
  if(network_port != NULL) {
    serialfd = open_network_device(network_port);
  } else {
    open_serial_device(serial_device, serial_baudrate, &serialfd);
  }
  //DEBUG("Connection to sensor network established\n");
  
  //--------------------------------------------------------------------------
  // SETUP LISTENER SOCKET
  // Get the listener
  if ((listenerfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("open_listener_sock: socket");
    exit(EXIT_FAILURE);
  }
  //DEBUG("Listener socket file descriptor %d\n", listenerfd);
  
  // Allow local address reuse
  if (setsockopt(listenerfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("open_listener_sock: setsockopt");
    exit(EXIT_FAILURE);
  }
  //DEBUG("Set listener socket options\n");
  
  // Bind
  server_addr.sin_family = AF_INET;          //! Use the network byte order
  server_addr.sin_addr.s_addr = INADDR_ANY;  //! The IP Address of the server machine
  server_addr.sin_port = htons(server_port); //! The port address to connect to the server
  memset(&(server_addr.sin_zero), '\0', 8);  //! Set the remaining fields of the struct to zero
  if (bind(listenerfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    printf("The port you have specified is probably in use.\n");
    printf("Try with another port\n");
    perror("Open_listener_sock: bind");
    exit(EXIT_FAILURE);
  }
  //DEBUG("Listener socket bound\n");
  //  printf("Server IP Address %s\n", inet_ntoa(server_addr.sin_addr));

  // Listen
  if (listen(listenerfd, LISTENER_BACKLOG) == -1) {
    perror("open_listener_sock: listen");
    exit(EXIT_FAILURE);
  }
  
  //--------------------------------------------------------------------------
  // MAIN EVENT LOOP
  FD_ZERO(&master_fds);             //! Clear the Master file descriptor list
  FD_ZERO(&read_fds);               //! Clear the Temp file descriptor list
  FD_SET(listenerfd, &master_fds);  //! Set the server listener fd
  FD_SET(serialfd, &master_fds);    //! Set the serial listener fd
  
  //! Track the largest file descriptor in the set
  fdmax = MAX(serialfd, listenerfd);
  
  // Main Dispatcher Loop
  printf("Starting sossrv on port %d\n", server_port);  
  printf("Server started ...\n");
  for(;;) {
    read_fds = master_fds; //! Copy the master file descriptor
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }
    
    // run through the existing connections looking for data to read
    for(curr_sock_fd = 0; curr_sock_fd <= fdmax; curr_sock_fd++) {
      if (FD_ISSET(curr_sock_fd, &read_fds)){
	//DEBUG("Event on fd %d\n", curr_sock_fd);
	// Event on listener port
	if (curr_sock_fd == listenerfd) {
	  // Handle new connections
	  addrlen = sizeof(remoteaddr);
	  if ((newfd = accept(listenerfd, (struct sockaddr *)&remoteaddr, (socklen_t *)&addrlen)) == -1)
	    perror("Sossrv: accept");
	  else {
	    FD_SET(newfd, &master_fds); // add to master set
	    fdmax = MAX(fdmax, newfd);
	    printf("Sossrv: Established new connection from %s on socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
	  }
	}
	
	// Event on serial port
	else if (curr_sock_fd == serialfd)
	  serial_pkt_handler();
	
	// Event on the other port
	else
	  network_pkt_handler();
      }
    }
  }
  return 0;
}


//---------------------------------------------------------------------------------
// SERIAL PACKET HANDLER
enum {
	SERIAL_RX_HDLC_START=0,	//! Starting a new connection
	SERIAL_RX_HDLC_RESTART,	//! Allow for repeated start symbols
	SERIAL_RX_HEADER,		//! Receive the SOS Message Header
	SERIAL_RX_DATA,		//! Receive the SOS Message Data
	SERIAL_RX_ESCAPE, //! Recieved HDLC Escape sequence
	SERIAL_RX_WAIT,		//! Waiting for data or start symbol
};

void serial_byte_handler(unsigned char byte);

int serial_pkt_handler()
{
	//unsigned char data[255];
	//int n = read(serialfd, data, 255);
	unsigned char data[1024];
	int i = 0;
	int n = read(serialfd, data, 1024);

	while (i<n) {
		if (outputOpts >= OUTPUT_DEBUG) {
			DEBUG("RX 0x%02X : ", data[i]);
		}
		serial_byte_handler(data[i]);
		i++;
	}
}

void serial_byte_handler(unsigned char rxbyte)
{
	int numrecv;
	static unsigned char serial_rx_state = SERIAL_RX_WAIT;
	static unsigned char serial_hdlc_state = HDLC_IDLE;
	static unsigned char saved_state = HDLC_IDLE;
	static unsigned char protocol = HDLC_SOS_MSG;
	
	static SOS_Message_t serialrxsosmsg;
	static unsigned char* pserialrx;
	static unsigned char* rx_buff = serialrxsosmsg.data;
	static unsigned char rx_len = 0;
	static int numserialrxbytes;
	static unsigned short rxCRCval;

	unsigned short computeCRCval;


	switch (serial_rx_state) {
		case SERIAL_RX_HDLC_START:
			{
				if (rxbyte != HDLC_FLAG) {
					if (outputOpts >= OUTPUT_DEBUG) {
						DEBUG("HCLC FLAG byte\n");
					}
					break;
				}
			}
			
		case SERIAL_RX_WAIT:
		case SERIAL_RX_HDLC_RESTART:
			{
				if (rxbyte == HDLC_FLAG) {
					if (outputOpts >= OUTPUT_DEBUG) {
						DEBUG("HCLC FLAG byte\n");
					}
					serial_rx_state = SERIAL_RX_HDLC_RESTART;
					break;
				}

				numserialrxbytes = 0;
				serial_hdlc_state = HDLC_DATA;
				protocol = rxbyte;

				switch (protocol) {
					case HDLC_SOS_MSG:
						{
							pserialrx = (unsigned char*)(&serialrxsosmsg);
							serial_rx_state = SERIAL_RX_HEADER;
							if (outputOpts >= OUTPUT_DEBUG) {
								DEBUG("protocol HDLC_SOS_MSG\n");
							}
							rx_len = 0;
							break;
						}
					case HDLC_RAW:
						{
							pserialrx = rx_buff;
							if (pserialrx == NULL) {
								printf("No free space to store incomming message.  Dropping!\n");
								serial_rx_state = SERIAL_RX_WAIT;
							} else {
								if (outputOpts >= OUTPUT_DEBUG) {
									DEBUG("protocol HDLC_RAW\n");
								}
								serial_rx_state = SERIAL_RX_DATA;
								rx_len = UART_MAX_MSG_LEN;
							}
							break;
						}
					default:
						{
							serial_rx_state = SERIAL_RX_WAIT;
							break;
						}
				}
				break;
			}
		case SERIAL_RX_HEADER:
			{
				if (rxbyte == HDLC_CTR_ESC) {
					saved_state = serial_hdlc_state;
					serial_hdlc_state = HDLC_ESCAPE;
					if (outputOpts >= OUTPUT_DEBUG) {
						DEBUG("ESC byte\n");
					}
					return;
				}

				if (serial_hdlc_state == HDLC_ESCAPE) {
					rxbyte ^= 0x20;
					serial_hdlc_state = saved_state;
				}

				if (outputOpts >= OUTPUT_DEBUG) {
					DEBUG("data byte 0x%02X\n", rxbyte);
				}

				pserialrx[numserialrxbytes++] = rxbyte;

				if (numserialrxbytes == SOS_MSG_HEADER_SIZE) {
					numserialrxbytes = 0;
					serial_rx_state = SERIAL_RX_DATA;
					pserialrx = rx_buff;
					rx_len = serialrxsosmsg.len;
					//DEBUG("recv header of length %d: expecting message of len = %d\n", SOS_MSG_HEADER_SIZE, serialrxsosmsg.len);
				}
				break;
			}
		case SERIAL_RX_DATA:
			{
				switch (rxbyte) {
					case HDLC_FLAG:
						{
							if (outputOpts >= OUTPUT_DEBUG) {
								DEBUG("HCLC FLAG byte\n");
							}
							if (protocol == HDLC_SOS_MSG) {
								computeCRCval = computeMsgCRC(protocol, &serialrxsosmsg);
								if ((serial_hdlc_state == HDLC_PADDING) && (computeCRCval == rxCRCval)) {
									if (outputOpts >= OUTPUT_QUIET) {
										printsosmsg(&serialrxsosmsg, rxCRCval, "Received from Serial, CRC OK!");
									}
									dispatch_sos_message(&serialrxsosmsg);
								} else {
									if (outputOpts >= OUTPUT_QUIET) {
										printsosmsg(&serialrxsosmsg, rxCRCval, "Received from Serial, CRC FAIL!");
										if (outputOpts >= OUTPUT_DEBUG) {
											DEBUG("CRC: recieved = 0x%04X, packet computed = 0x%04X\n", rxCRCval, computeCRCval);
										}
									}
								}
							} else {
								// dispatch to raw handler
								if (outputOpts >= OUTPUT_QUIET) {
									printrawmsg(serialrxsosmsg.data, numserialrxbytes, "Recieved from Serial, RAW Mode!");
								}
							}
							serial_rx_state = SERIAL_RX_WAIT;
							serial_hdlc_state = HDLC_IDLE;
							break;
						}
					case HDLC_CTR_ESC:
						{
							saved_state = serial_hdlc_state;  //! may need to escape a crc byte
							serial_hdlc_state = HDLC_ESCAPE;
							if (outputOpts >= OUTPUT_DEBUG) {
								DEBUG("ESC byte\n");
							}
							break;
						}
						/* not a Flag Sequence or Escape Sequence handle based on HDLC state */
					default:
						{
							if (serial_hdlc_state == HDLC_ESCAPE) {
								rxbyte ^= 0x20;
								serial_hdlc_state = saved_state;
							}

							switch (serial_hdlc_state) {
								case HDLC_DATA:
									{
										if (numserialrxbytes < rx_len) {
											if (outputOpts >= OUTPUT_DEBUG) {
												DEBUG("data byte 0x%02X\n", rxbyte);
											}
											pserialrx[numserialrxbytes++] = rxbyte;
										} else {
											if (outputOpts >= OUTPUT_DEBUG) {
												DEBUG("crc low 0x%02X\n", rxbyte);
											}
											rxCRCval = (unsigned short)(rxbyte);
											serial_hdlc_state = HDLC_CRC;
										}
										break;
									}
								case HDLC_CRC:
									{
										if (outputOpts >= OUTPUT_DEBUG) {
											DEBUG("crc high 0x%02X\n", rxbyte);
										}
										rxCRCval = (0xff00&(((unsigned short)(rxbyte))<<8)) + rxCRCval;
										serial_hdlc_state = HDLC_PADDING;
										break;
									}
								default: // what are we doing here??
									{ // start looking for the next start symbol
										serial_rx_state = SERIAL_RX_HDLC_START;
										serial_hdlc_state = HDLC_IDLE;
									}
							}
						}
				}
			}
	}
}


//---------------------------------------------------------------------------------
// SERIAL RX PACKET DISPATCHER
int dispatch_sos_message(SOS_Message_t* psosmsg)
{
	int i;
	int netwrbytes;
	for (i = 0; i <= fdmax; i++){
		if (FD_ISSET(i, &master_fds) && (i != serialfd) && (i != listenerfd)){
			netwrbytes = writen(i, psosmsg, psosmsg->len + SOS_MSG_HEADER_SIZE);
			if ((netwrbytes < 0) || (netwrbytes < (psosmsg->len + SOS_MSG_HEADER_SIZE))){
				if (netwrbytes < 0){
					perror("Sossrv: Send");
				}
				else{
					printf("Sossrv: Socket %d hung up\n", i);
				}
				close(i);
				FD_CLR(i, &master_fds);
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------------------
// NETWORK PACKET HANDLER
int network_pkt_handler()
{
  //unsigned char netrxbuf[256];    //! Buffer for data received over the network
  unsigned char netrxbuf[1024];    //! Buffer for data received over the network
  int netrxbytes;                 //! Number of bytes received over the network
  SOS_Message_t* psosmsg;         //! SOS SOS_Message_t Pointer
	static unsigned char frameByte = HDLC_FLAG;
	static unsigned char protocolByte = HDLC_SOS_MSG;
  unsigned short txbuffCRC;       //! CRC of the outgoing message
  unsigned char txCRC[2];        //! CRC Bytes (To take care of endianness)
	
  // We first read the message header
  netrxbytes = readn(curr_sock_fd, netrxbuf, SOS_MSG_HEADER_SIZE);
	if ((netrxbytes < 0) || (netrxbytes < SOS_MSG_HEADER_SIZE)) {
		// Connection closed by client
		if (netrxbytes < 0)
			perror("Sossrv: Receive");
		else
			printf("Sossrv: Socket %d hung up\n", curr_sock_fd);
		close(curr_sock_fd);
		FD_CLR(curr_sock_fd, &master_fds);
		return -1;
	} else {
		psosmsg = (SOS_Message_t*)netrxbuf;
		if (psosmsg->len > 0){
			netrxbytes = readn(curr_sock_fd, &psosmsg->data[0], psosmsg->len);
			if ((netrxbytes < 0) || (netrxbytes < psosmsg->len)){
				if (netrxbytes < 0)
					perror("Sossrv: Receive");
				else
					printf("Sossrv: Socket %d hung up\n", curr_sock_fd);
				close(curr_sock_fd);
				FD_CLR(curr_sock_fd, &master_fds);
				return -1;
			}
		}
		//DEBUG("Received a send message packet\n");
		txbuffCRC = computeMsgCRC(protocolByte, psosmsg);
		txCRC[0] = (unsigned char) txbuffCRC;
		txCRC[1] = (unsigned char)(txbuffCRC >> 8);

		printsosmsg(psosmsg, txbuffCRC, "Received from Desktop");

		// Can block -- Its a slow serial link
		writeb(serialfd, &frameByte, 1);
		// for now we will only support transmiting of sos_msgs
		writeb(serialfd, &protocolByte, 1);
		write_string(serialfd, psosmsg, SOS_MSG_HEADER_SIZE + psosmsg->len);
		write_string(serialfd, &txCRC, 2);
		writeb(serialfd, &frameByte, 1);

	}
}


unsigned short computeMsgCRC(unsigned char protocol, SOS_Message_t* psosmsg)
{
  unsigned char i;
  unsigned char* txbuffptr;

  unsigned short runningCRC = crcByte(0, protocol);

  txbuffptr = (unsigned char*) psosmsg;
  for (i = 0; i < SOS_MSG_HEADER_SIZE; i++)
    runningCRC = crcByte(runningCRC, txbuffptr[i]);
  
  for (i = 0; i < psosmsg->len; i++)
    runningCRC = crcByte(runningCRC, psosmsg->data[i]);
  
  return runningCRC;
}


unsigned short crcByte(unsigned short crc, unsigned char b)
{
  unsigned char i;
  crc = crc ^ b << 8;
  i = 8;
  do
    if (crc & 0x8000)
      crc = crc << 1 ^ 0x1021;
    else
      crc = crc << 1;
  while (--i);
  return crc;
}


int printrawmsg(unsigned char *buff, int len, char *header) {
	int i=0;
	//printf("--------------------------------\n");
	//printf("--- %s ---\n", header);
	printf("\n--- %s ---\n", header);
	for (i=0; i<len; i++){
		if ((i!=0) && !(i%16)) printf("\n");
		printf("0x%02X ", buff[i]);
	}
	printf("\n");
}


int printsosmsg(SOS_Message_t* psosmsg, unsigned short rxCRCval, char *header)
{
	int i;
	//printf("--------------------------------\n");
	//printf("--- %s ---\n", header);
	printf("\n--- %s ---\n", header);
	if (outputOpts >= OUTPUT_DEFAULT) {
		printf("Dest Mod Id: 0x%02X\n", psosmsg->did);
		printf("Src  Mod Id: 0x%02X\n", psosmsg->sid);
#ifdef BBIG_ENDIAN
		printf("Dest Addr  : 0x%02X\n", Flip_int16(psosmsg->daddr));
		printf("Src  Addr  : 0x%02X\n", Flip_int16(psosmsg->saddr));
#else 
		printf("Dest Addr  : 0x%02X\n", psosmsg->daddr);
		printf("Src  Addr  : 0x%02X\n", psosmsg->saddr);
#endif
		printf("Msg Type   : %d\n", psosmsg->type);
		printf("Msg Length : %d\n", psosmsg->len);
		printf("Msg Data:\n");
	} else if (outputOpts > OUTPUT_QUIET) {
		printf("did: 0x%02X sid: 0x%02X ", psosmsg->did, psosmsg->sid);
#ifdef BBIG_ENDIAN
		printf("daddr: 0x%02X saddr: 0x%02X ", Flip_int16(psosmsg->daddr), Flip_int16(psosmsg->saddr));
#else 
		printf("daddr: 0x%02X saddr: 0x%02X ", psosmsg->daddr, psosmsg->saddr);
#endif
		printf("type: %d len:%d\n", psosmsg->type, psosmsg->len);
		printf("data: ");
	}
	for (i = 0; i < psosmsg->len; i++){
		if ((i!=0) && !(i%16)) printf("\n");
		printf("0x%02X ",psosmsg->data[i]);
	}
	printf("\nCRC: 0x%04X\n", rxCRCval);
	return 0;
}

