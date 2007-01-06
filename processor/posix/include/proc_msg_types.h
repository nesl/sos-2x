/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    device related messages
 * @author   Naim Busek	(naim@gmail.com)
 * @version  0.1
 *
 */
#ifndef _PROC_MSG_TYPES_H_
#define _PROC_MSG_TYPES_H_
#include <message_types.h>

/**
 * @brief device msg_types list
 */
enum {
	/* i2c_system msgs */
 MSG_I2C_SEND_DONE = PROC_MSG_START, //!< I2C send done
 MSG_I2C_READ_DONE, //!< I2C read Done     
 /* uart_system msgs */
 MSG_UART_SEND_DONE, //!< uart send done
 MSG_UART_READ_DONE, //!< uart read Done     
 MSG_EXFLASH_DESELECT,
 MSG_EXFLASH_AVAIL,
 MSG_EXFLASH_READDONE,
 MSG_EXFLASH_CRCDONE,
 MSG_EXFLASH_WRITEDONE,
 MSG_EXFLASH_SYNCDONE,
 MSG_EXFLASH_FLUSHDONE,
 MSG_EXFLASH_ERASEDONE,

};

#endif /* _PROC_MSG_TYPES_H_ */

