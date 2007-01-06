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
#include <time.h>

#include <hdlc.h>

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
				perror("writen");
				return -1;
			}
		}
		nleft -= nwrite;
		ptr += nwrite;
	}
	return nbytes;
}


//------------------------------------------------------------
// WRITE ONE SYMBOL - BLOCKING CALL
int writeb(int fd, void *symbol, int symbol_len)
{
	void* ptr;
	int nleft, nwrite;
	//	int i;

	ptr = symbol;
	nleft = symbol_len;
#if 0
	for (i=0; i<symbol_len; i++) {
		printf("writen: 0x%02X\n", ((unsigned char *)(ptr))[i]);
	}
#endif
	while (nleft > 0){
		nwrite = write(fd, ptr, nleft);
		if (nwrite < 0) {
			if (errno == EINTR) {
				nwrite = 0;
			} else {
				perror("writen");
				return -1;
			}
		}
		nleft -= nwrite;
		ptr += nwrite;
	}
	return symbol_len;
}

//------------------------------------------------------------
// WRITE N BYTES WITH 1 ms DELAY BETWEEN EVERY BYTE - BLOCKING CALL
int writeslown(int fd, void *vptr, int nbytes)
{
  struct timespec t1, t2;
  void* ptr;
  int nleft, nwrite;
  ptr = vptr;
  nleft = nbytes;
  t1.tv_sec = 0;
  t1.tv_nsec = 1000000;
  while (nleft > 0){
    nanosleep(&t1, &t2);
    while (t2.tv_nsec != 0){
      t1.tv_nsec = t2.tv_nsec;
      nanosleep(&t1, &t2);
    }
    nwrite = write(fd, ptr, 1);
    printf("%02X ", ((char*)ptr)[0]);
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


//------------------------------------------------------------
// WRITE N BYTES one char at a time - BLOCKING CALL
int write_string(int fd, void *vptr, int nbytes)
{
	void* ptr;
	int nleft, nwrite;
	unsigned char symbol[2];

	ptr = vptr;
	nleft = nbytes;
	symbol[0] = HDLC_CTR_ESC;
	while (nleft > 0) {
		if ((((unsigned char *)(ptr))[0] == HDLC_FLAG) ||
				(((unsigned char *)(ptr))[0] == HDLC_CTR_ESC) ||
				(((unsigned char *)(ptr))[0] == HDLC_EXT)) {
			symbol[1] = 0x20 ^ ((unsigned char *)(ptr))[0];
			nleft++;
			nwrite = writeb(fd, &symbol, 2);
		} else {  // optmize here send longest run of non control chars
			nwrite = writeb(fd, ptr, 1);
		}
		if (nwrite < 0) {
			perror("write_pkt");
			return -1;
		}
		nleft -= nwrite;
		ptr++;
	}
	return nbytes;
}

