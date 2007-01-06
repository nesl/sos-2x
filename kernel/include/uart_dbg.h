/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief    header file for uart debugging
 * @author   Naim Busek
 */


#ifndef _UART_DBG_H
#define _UART_DBG_H

#include <sos_info.h>
#include <malloc.h>
#include <message.h>

/* id must be a unique char string that will be appended to var instantation
 * to prevent namespace conflicts name */
#ifdef UART_DEBUG
#define UART_DBG_MSG_LEN 4
#define UART_DBG(name, flg0, flg1, flg2, flg3, pid) { \
	uint8_t *uart_msg##name = ker_malloc(UART_DBG_MSG_LEN, pid); \
	if(uart_msg##name) { \
		uart_msg##name[0] = (flg0); \
		uart_msg##name[1] = (flg1); \
		uart_msg##name[2] = (flg2); \
		uart_msg##name[3] = (flg3); \
		post_uart(pid, pid, 69, UART_DBG_MSG_LEN, uart_msg##name, SOS_MSG_RELEASE, UART_ADDRESS); \
	} else { \
		ker_free(uart_msg##name); \
	} \
}
#else
#define UART_DBG(name, flg0, flg1, flg2, flg3, pid)
#endif

#endif

