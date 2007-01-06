/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * \file sock_utils.c
 * \brief Commonly used socket utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sock_utils.h>




//------------------------------------------------------------
// READ N BYTES - BLOCKING CALL
int readn(int fd, void *vptr, int nbytes)
{
  void* ptr;
  int nleft, nread;
  
  ptr = vptr;
  nread = 0;
  nleft = nbytes;
  
  while (nleft > 0){
    nread = read(fd, ptr, nleft);
    if (nread < 0) {
			if (errno == EINTR) {
				nread = 0;
			} else {
				perror("readn");
				return -1;
			}
		}
		else if (nread == 0) 
			break;

		nleft -= nread;
		ptr += nread;
	}
	return (nbytes-nleft); 
}


//------------------------------------------------------------
// WRITE N BYTES - BLOCKING CALL
int writen(int fd, void *vptr, int nbytes)
{
  void* ptr;
	int nleft, nwrite;

	ptr = vptr;
	nleft = nbytes;
	while (nleft > 0) {
		nwrite = write(fd, ptr, nleft);
		if (nwrite < 0){
			if (errno == EINTR) {
				nwrite = 0;
			} else {
				perror("writen:");
				return -1;
			}
		}
		nleft -= nwrite;
		ptr += nwrite;
	}
	return nbytes;
}
