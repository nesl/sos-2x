/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    MSP430 Processor Message Types
 * @author   Ram Kumar {ram@ee.ucla.edu}
 * $Id: proc_msg_types.h,v 1.1 2006/02/12 22:25:28 ram Exp $
 */

#ifndef _PROC_MSG_TYPES_H_
#define _PROC_MSG_TYPES_H_
#include <message_types.h>

/**
 * @brief device msg_types list
 */
enum {
 /* uart_system msgs */
 MSG_UART_SEND_DONE = PROC_MSG_START, //!< uart send done
 MSG_UART_READ_DONE, //!< uart read Done
};

#endif /* _PROC_MSG_TYPES_H_ */

