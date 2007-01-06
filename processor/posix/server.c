/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#include <message_queue.h>
#include <net_stack.h>

#include <hardware.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>

#include <server.h>

/*
 * @brief Serer port number. Set to -1 to disable server as default
 *
 */
int server_tcp_port = -1;

/**
 * @brief Server receiver side
 */
static void server_recv_thread(int fd)
{
	uint8_t d[1024];
	int cnt;
	//sched_yield();
	while( 1 ) {
		cnt = read(fd, d, SOS_MSG_HEADER_SIZE);
		if (cnt == 0) {	// The socket is closed!
			interrupt_remove_read_fd(fd);
			close(fd);
			break;
		}
		if(cnt != SOS_MSG_HEADER_SIZE)
			break;
		Message *buf = (Message*)d;
		Message *m = msg_create();
		if (m == NULL)
			break;
		uint8_t *data = ker_malloc(buf->len,UART_PID);
		if (data == NULL) {
			msg_dispose(m);
			break;
		}
		memcpy(m,buf,SOS_MSG_HEADER_SIZE);
		m->daddr = entohs(m->daddr);
		m->saddr = entohs(m->saddr);
		m->data = data;
		m->flag = SOS_MSG_RELEASE;
		while ((cnt = read(fd, m->data, m->len)) < 0);
		if (cnt != m->len) {
			msg_dispose(m);
			break;
		}
		handle_incoming_msg(m, SOS_MSG_UART_IO);
	}
}

/**
 * @brief Server listen side
 */
static void server_listen_thread(int fd)
{
	struct sockaddr_in remoteaddr;  //! Client address
	socklen_t addrlen;
	int server_socket;

	addrlen = sizeof(remoteaddr);
	if ((server_socket = accept(fd, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
		perror("Accept in server.c");
		interrupt_remove_read_fd(fd);
		close(fd);
		return;
	} else {
		DEBUG("Sossrv: Established new connection from %s\n",
			inet_ntoa(remoteaddr.sin_addr));
	}
	interrupt_add_read_fd(server_socket, server_recv_thread);
}

void server_init(void)
{
	int listen_sock = -1;
	struct sockaddr_in server_addr;

	//! check to see whether uart is required
	if(server_tcp_port < 0) return;
	DEBUG("Starting the server on port: %d\n", server_tcp_port);
	//! open TCP server
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock < 0) {
		perror("cannot open socket for Server");
		exit(1);
	}
	//! waiting connection
	server_addr.sin_family = AF_INET;          //! Use the network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;  //! The IP Address of the server machine
	server_addr.sin_port = htons(server_tcp_port); //! The port address to connect to the server
	memset(&(server_addr.sin_zero), '\0', 8);  //! Set the remaining fields of the struct to zero

	if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("cannot bind server TCP port");
		close(listen_sock);
		exit(1);
	}

	// Listen
	if (listen(listen_sock, 1) < 0) {
		perror("listen failed");
		close(listen_sock);
		exit(1);
	}
	DEBUG("listening Server connection from port %d\n", server_tcp_port);
	interrupt_add_read_fd(listen_sock, server_listen_thread);
}

void server_terminate( void )
{
#if 0
	if( server_socket != -1 ) {		// Should we remove from interrupt table?
		close(server_socket);
	}
	if( listen_sock != -1 ) {
		close(listen_sock);
	}
#endif
}

