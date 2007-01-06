/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file uart_system.h
 * @brief uart SOS interface
 * @author Naim Busek
 **/

#ifndef _UART_SYSTEM_H
#define _UART_SYSTEM_H
#include <proc_msg_types.h>

// flags for uart_system
#define UART_SYS_SOS_MSG_FLAG 0x80 // shared with i2c driver
#define UART_SYS_TX_FLAG      0x40 // shared with i2c driver
#define UART_SYS_RSVRD_4_FLAG 0x20
#define UART_SYS_RSVRD_3_FLAG 0x10
#define UART_SYS_RSVRD_2_FLAG 0x08
#define UART_SYS_RSVRD_1_FLAG 0x04
#define UART_SYS_RSVRD_0_FLAG 0x02
#define UART_SYS_ERROR_FLAG   0x01 // shared with i2c driver

#define UART_SYS_NULL_FLAG    0x00

#define UART_SYS_SHARED_FLAGS_MSK 0xD1

int8_t uart_system_init();

void uart_read_done(uint8_t len, uint8_t status);
void uart_send_done(uint8_t status);

#ifndef _MODULE_
/**
 * @brief UART HPL Related Functions
 */
int8_t ker_uart_reserve_bus(uint8_t calling_id, uint8_t flags);
int8_t ker_uart_release_bus(uint8_t calling_id);

int8_t ker_uart_send_data(
        uint8_t *buff, 
        uint8_t msg_size, 
        uint8_t calling_id);
int8_t ker_uart_read_data(
        uint8_t read_size, 
        uint8_t calling_id);
#endif
#endif // _UART_SYSTEM_H

