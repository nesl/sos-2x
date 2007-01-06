/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#ifndef _UART_HAL_H_
#define _UART_HAL_H_

/**
 * @brief UART_HAL config options
 */
#define SMCLK_BAUD_57_6k_UMCTL  0x84
#define SMCLK_BAUD_57_6k_UBR1   0x00
#define SMCLK_BAUD_57_6k_UBR0   0x12

#define uart_checkError()				 ((U1RCTL & RXERR) ? 1 : 0)

#define uart_checkFramingError() ((U1RCTL & FE) ? 1 : 0)
#define uart_checkParityError()  ((U1RCTL & PE) ? 1 : 0)
#define uart_checkOverrunError() ((U1RCTL & OE) ? 1 : 0)
#define uart_checkBreakCond()    ((U1RCTL & BRK) ? 1 : 0)

#define uart_getByte()           (U1RXBUF)
#define uart_setByte(b)			     (U1TXBUF = (b))

#define uart_recv_interrupt()    interrupt (USART1RX_VECTOR) uart_rx_isr()
#define uart_send_interrupt()    interrupt (USART1TX_VECTOR) uart_tx_isr()

#define uart_disable()            IE2 &= ~((URXIE1 | UTXIE1))

#define uart_disable_tx()         IE2 &= (~UTXIE1)
#define uart_enable_tx()          IE2 |= UTXIE1

#define uart_disable_rx()         IE2 &= (~URXIE1)
#define uart_enable_rx()          IE2 |= URXIE1

#define uart_is_disabled()        ((IE2 & UTXIE1) ? 0 : 1)

/**
 * @brief UART HAL init
 */
extern void uart_hardware_init(void);

#endif // _UART_HAL_H_


