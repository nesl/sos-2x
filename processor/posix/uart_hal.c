/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */


#include <hardware.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>

#include <uart.h>
#include "uart_hal.h"

/**
 * @brief UART file id
 */
extern int uart_tcp_port;

/**
 * @brief UART sender side
 */
static volatile bool uart_status = false;
static int uart_socket = -1;
static int listen_sock = -1;

void uart_disable_tx()
{
	uart_status = false;
}

void uart_enable_tx()
{
	uart_status = true;
}

bool uart_is_disabled()
{
	bool ret_val;

	ret_val = !uart_status;
	return ret_val;
}

void uart_setByte(uint8_t b)
{
	if(uart_socket >= 0) {
		if(write(uart_socket, &b, 1) != 1) {
			DEBUG("remote connection to UART broken, exiting...\n");
			hardware_exit(1);
		}
	}
	interrupt_add_callbacks(uart_send_int_);
}


/**
 * @brief UART receiver side
 */
static uint8_t recv_buf;
static void uart_recv_thread(int fd)
{
	uint8_t d[1024];
	int cnt;
	int i = 0;

	while( true ) {
		cnt = read(uart_socket, d, 1024);
		if(cnt > 0){
			for(i = 0; i < cnt; i++){
				recv_buf = d[i];
				uart_recv_int_();
			}
		} else {
			break;
		}
	}
}

uint8_t uart_getByte()
{
	return recv_buf;
}

void uart_hardware_init(void)
{
	socklen_t addrlen;
	struct sockaddr_in server_addr;
	struct sockaddr_in remoteaddr;  //! Client address

	//! check to see whether uart is required
	if(uart_tcp_port < 0) return;
	//! open TCP server
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_sock < 0) {
		perror("cannot open socket for UART");
		exit(1);
	}
	//! waiting connection
	server_addr.sin_family = AF_INET;          //! Use the network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;  //! The IP Address of the server machine
	server_addr.sin_port = htons(uart_tcp_port); //! The port address to connect to the server
	memset(&(server_addr.sin_zero), '\0', 8);  //! Set the remaining fields of the struct to zero

	if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("cannot bind UART TCP port");
		close(listen_sock);
		exit(1);
	}

	// Listen
	if (listen(listen_sock, 1) < 0) {
		perror("listen failed");
		close(listen_sock);
		exit(1);
	}
	DEBUG("listening UART connection from port %d\n", uart_tcp_port);

	addrlen = sizeof(remoteaddr);
	if ((uart_socket = accept(listen_sock, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
		perror("Accept in uart.c");
		close(listen_sock);
		exit(1);
	} else {
		DEBUG("Sossrv: Established new connection from %s\n",
				inet_ntoa(remoteaddr.sin_addr));
	}

	interrupt_add_read_fd(uart_socket, uart_recv_thread);
}

void uart_hardware_terminate( void )
{
	if( uart_socket != -1 ) {
		close(uart_socket);
	}
	if( listen_sock != -1 ) {
		close(listen_sock);
	}
}

