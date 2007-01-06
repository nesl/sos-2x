/* -*-C-*- */
/**
 * \file sock_utils.h
 * \brief Socket Utilities
 */

#ifndef _SOCK_UTILS_H_
#define _SOCK_UTILS_H_


int readn(int fd, void *vptr, int nbytes);
int writen(int fd, void *vptr, int nbytes);
// This write utility introduces a 1 ms delay between every byte
// Used in the JTAG mode when the target is slower than normal
int writeslown(int fd, void *vptr, int nbytes);

#endif //_SOCK_UTILS_H_
