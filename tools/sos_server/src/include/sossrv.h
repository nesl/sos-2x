/** -*-C-*- */

/**
 * \file sossrv.h
 * \brief The include file for the sossrv API
 */

#ifndef _SOSSRV_H_
#define _SOSSRV_H_

#include <stddef.h>
#define IGNORE_SUB_PIDS

#include <message_types.h>

#define DEBUG(arg...)  printf(arg)
//#define DEBUG(arg...)

#define DEFAULT_SERVER_PORT 7915          //! Server Port
#define DEFAULT_SERIAL_PORT "/dev/ttyUSB0" //! Serial port of the SOS NIC
#define DEFAULT_BAUDRATE    57600        //! Baudrate of the SOS NIC

/* enum  */
/*   { */
/*     KER_MOD_MAX_PID    = 63,      //! highest pid kernel module can use */
/*     DEV_MOD_MIN_PID    = 64,      //! pids for device modules */
/*     APP_MOD_MIN_PID    = 128,     //! pids for applications */
/*   }; */


enum {
	OUTPUT_SILENT  = 0x00,
	OUTPUT_QUIET   = 0x04,
	OUTPUT_DEFAULT = 0x05,
	OUTPUT_DEBUG   = 0x0f,
};


#define MAX_DATA_SIZE 1024

//typedef unsigned char sos_pid_t;
/**
 * @brief message
 */
typedef struct SOS_Message_t{
  sos_pid_t  did;                              //!< module destination id
  sos_pid_t  sid;                              //!< module source id
  unsigned short daddr;                        //!< node destination address
  unsigned short saddr;                        //!< node source address
  unsigned char  type;                         //!< module specific message type
  unsigned char  len;                          //!< payload length
  unsigned char  data[MAX_DATA_SIZE];          //!< actual payload
} __attribute__ ((packed)) SOS_Message_t;
    
//#define SOS_MSG_HEADER_SIZE (offsetof(struct SOS_Message_t, data))


#endif //_SOSSRV_H_

