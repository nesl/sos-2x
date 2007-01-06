/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
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
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
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

#ifndef _UART_HAL_H
#define _UART_HAL_H

/**
 * @brief UART_HAL config options
 */
// assuming 7.3728MHz system clock
// normal operation
#define BAUD_2400	191	//  0.0%
#define BAUD_4800	95	//  0.0%
#define BAUD_9600	47	//  0.0%
#define BAUD_14_4k	31	//  0.0%
#define BAUD_19_2k	23	//  0.0%
#define BAUD_28_8k	15	//  0.0%
#define BAUD_38_4k	11	//  0.0%
#define BAUD_57_6k	7	//  0.0%
#define BAUD_76_8k	5	//  0.0%
#define BAUD_115_2k	3	//  0.0%
#define BAUD_230_4k	1	//  0.0%
#define BAUD_250k	1	// -7.8%
#define BAUD_0_5M	0	// -7.8%
// double speed clock (U2X = 1)
#define BAUD_2400_U2X	383	//  0.0%
#define BAUD_4800_U2X	191	//  0.0%
#define BAUD_9600_U2X	95	//  0.0%
#define BAUD_14_4k_U2X	63	//  0.0%
#define BAUD_19_2k_U2X	47	//  0.0%
#define BAUD_28_8k_U2X	31	//  0.0%
#define BAUD_38_4k_U2X	23	//  0.0%
#define BAUD_57_6k_U2X	15	//  0.0%
#define BAUD_76_8k_U2X	11	//  0.0%
#define BAUD_115_2k_U2X	7	//  0.0%
#define BAUD_230_4k_U2X	3	//  0.0%
#define BAUD_250k_U2X	3	// -7.8%
#define BAUD_0_5M_U2X	1	// -7.8%
#define BAUD_1M_U2X		0	// -7.8%


/**
 * @brief check uart frame error and data overrun error
 * @return masked errors
 */
#ifndef USE_UART1
#define uart_checkError()        (UCSR0A & ((1<<FE)|(1<<DOR)|(1<<UPE)))

#define uart_checkFramingError() (UCSR0A & (1<<FE))
#define uart_checkOverrunError() (UCSR0A & (1<<DOR))
#define uart_checkParityError()  (UCSR0A & (1<<UPE))

#define uart_getByte()          (UDR0)
#define uart_setByte(b)         (UDR0 = (b))

#define uart_recv_interrupt()   SIGNAL(SIG_USART0_RECV)
#define uart_send_interrupt()   SIGNAL(SIG_USART0_TRANS)

#define uart_disable()			UCSR0B &= ((unsigned char)~((1<<(RXCIE))|(1<<(TXCIE))))

#define uart_disable_tx()       UCSR0B &= ((unsigned char)~(1<<(TXCIE)))
#define uart_enable_tx()        UCSR0B |= (1<<(TXCIE))

#define uart_disable_rx()       UCSR0B &= ((unsigned char)~(1<<(RXCIE)))
#define uart_enable_rx()        UCSR0B |= (1<<(RXCIE))

#define uart_is_disabled()      ((UCSR0B & (1 << TXCIE)) ? 0 : 1)
#else

//
// UART1
//

#define uart_checkError()        (UCSR1A & ((1<<FE)|(1<<DOR)|(1<<UPE)))

#define uart_checkFramingError() (UCSR1A & (1<<FE))
#define uart_checkOverrunError() (UCSR1A & (1<<DOR))
#define uart_checkParityError()  (UCSR1A & (1<<UPE))

#define uart_getByte()          (UDR1)
#define uart_setByte(b)         (UDR1 = (b))

#define uart_recv_interrupt()   SIGNAL(SIG_USART1_RECV)
#define uart_send_interrupt()   SIGNAL(SIG_USART1_TRANS)

#define uart_disable()			UCSR1B &= ((unsigned char)~((1<<(RXCIE))|(1<<(TXCIE))))

#define uart_disable_tx()       UCSR1B &= ((unsigned char)~(1<<(TXCIE)))
#define uart_enable_tx()        UCSR1B |= (1<<(TXCIE))

#define uart_disable_rx()       UCSR1B &= ((unsigned char)~(1<<(RXCIE)))
#define uart_enable_rx()        UCSR1B |= (1<<(RXCIE))

#define uart_is_disabled()      ((UCSR1B & (1 << TXCIE)) ? 0 : 1)

#endif


/**
 * @brief UART_HAL init
 */
int8_t uart_hardware_init(void);

#endif // _UART_HAL_H


