/*
 * Copyright (c) 2005 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * @author  Andrew Barton-Sweeney   (abs@cs.yale.edu)
 */

#ifndef _UART_HAL_H
#define _UART_HAL_H

#include <pxa27x_registers.h>

enum {
  UART_BAUD_300 = 1,
  UART_BAUD_1200 = 2,
  UART_BAUD_2400 = 3,
  UART_BAUD_4800 = 4,
  UART_BAUD_9600 = 5,
  UART_BAUD_19200 = 6,
  UART_BAUD_38400 = 7,
  UART_BAUD_57600 = 8,
  UART_BAUD_115200 = 9,
  UART_BAUD_230400 = 10,
  UART_BAUD_460800 = 11,
  UART_BAUD_921600 = 12
};

/**
 * @brief UART_HAL init
 */
int8_t uart_hardware_init(void);
//extern void uart_hardware_init(void);

/**
 * @brief SOS UART Interrupts
 */
extern void uart_recv_int_(void);
extern void uart_send_int_(void);
#define uart_recv_interrupt()      void uart_recv_int_(void)
#define uart_send_interrupt()      void uart_send_int_(void)

/**
 * @brief UART Control
 */
#define uart_enable_rx()
#define uart_disable_rx()
int uart_is_disabled();
void uart_enable_tx();
void uart_disable_tx();

/**
 * @brief UART Send and Receive Character
 */
extern int uart_putchar(char c);
char uart_getByte();
void uart_setByte(char b);

/**
 * @brief check uart frame error and data overrun error
 * @return masked errors
 */
char uart_checkError();

#endif // _UART_HAL_H
