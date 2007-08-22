/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    header file for uart debugging
 * @author   Naim Busek
 */


#ifndef _UART_DBG_H
#define _UART_DBG_H

#include <sys_module.h>

/* id must be a unique char string that will be appended to var instantation
 * to prevent namespace conflicts name */
#ifdef UART_DEBUG
#define UART_DBG_MSG_LEN (4*sizeof(uint32_t))
#define UART_DBG(flg0, flg1, flg2, flg3) { \
	uint32_t *uart_msg = (uint32_t*)sys_malloc(UART_DBG_MSG_LEN); \
	if(uart_msg) { \
		uart_msg[0] = (flg0); \
		uart_msg[1] = (flg1); \
		uart_msg[2] = (flg2); \
		uart_msg[3] = (flg3); \
		sys_post_uart(0, UART_DBG_MSG_LEN, uart_msg, SOS_MSG_RELEASE, BCAST_ADDRESS); \
	} else { \
		sys_free(uart_msg##name); \
	} \
}
#else
#define UART_DBG(flg0, flg1, flg2, flg3)
#endif

#endif

