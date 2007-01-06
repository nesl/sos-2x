/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    device related messages
 * @author   Naim Busek	(naim@gmail.com)
 * @version  0.1
 *
 */
#ifndef _PLAT_MSG_TYPES_H_
#define _PLAT_MSG_TYPES_H_
#include <message_types.h>

/**
 * @brief device msg_types list
 */
enum {
	/* PLAT_MSG_START == 0x80 (128) */
	/* spi msgs */
	MSG_SPI_SEND_DONE=PLAT_MSG_START, //!< SPI send done
	MSG_SPI_READ_DONE, //!< SPI read done
	MSG_SPI_FREE, //!< SPI notify blocked module
	/* mux msgs */
	MSG_MUX_SET_DONE,
	MSG_MUX_READ_DONE,
	MSG_MUX_FAIL,
	/* ltc6915_amp msgs */
	MSG_VAR_PREAMP_DONE,
	MSG_VAR_PREAMP_FAIL,

};

#endif /* _PLAT_MSG_TYPES_H_ */

