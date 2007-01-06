/* -*-C-*- */
/**
 * \file parsecmd.h
 * \brief The API for the command line parser utility
 */


#ifndef _PARSECMD_H_
#define _PARSECMD_H_

int parsecmdline(int argc, char *argv[], int* pServerPort, char** pSerialPort, int* pBaudRate, char** networkPort, int* reducedOutput);
int printuage();

#endif //_PARSECMD_H_
