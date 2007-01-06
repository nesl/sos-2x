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

#ifndef _HARDWARE_ONCE
#define _HARDWARE_ONCE

/* Global interrupt priority table.
 *    Table is indexed by the Peripheral ID (PPID). Priorities are 0 - 39
 *    where 0 is the highest.  Priorities MUST be unique. 0XFF = invalid/unassigned
 */
const uint8_t SOS_IRP_TABLE[] = { 0xFF, // PPID  0 SSP_3 Service Req
				   0xFF, // PPID  1 MSL
				   0xFF, // PPID  2 USBH2
				   0xFF, // PPID  3 USBH1
				   0xFF, // PPID  4 Keypad
				   0xFF, // PPID  5 Memory Stick
				   0xFF, // PPID  6 Power I2C
				   0x01, // PPID  7 OST match Register 4-11
				   0x02, // PPID  8 GPIO_0
				   0x03, // PPID  9 GPIO_1
				   0x04, // PPID 10 GPIO_x
				   0x08, // PPID 11 USBC
				   0xFF, // PPID 12 PMU
				   0xFF, // PPID 13 I2S
				   0xFF, // PPID 14 AC '97
				   0xFF, // PPID 15 SIM status/error
				   0xFF, // PPID 16 SSP_2 Service Req
				   0xFF, // PPID 17 LCD Controller Service Req
				   0xFF, // PPID 18 I2C Service Req
				   0xFF, // PPID 19 TX/RX ERROR IRDA
				   0x07, // PPID 20 TX/RX ERROR STUART
				   0xFF, // PPID 21 TX/RX ERROR BTUART
				   0x06, // PPID 22 TX/RX ERROR FFUART
				   0xFF, // PPID 23 Flash Card status/Error Detect
				   0x05, // PPID 24 SSP_1 Service Req
				   0x00, // PPID 25 DMA Channel Service Req
				   0xFF, // PPID 26 OST equals Match Register 0
				   0xFF, // PPID 27 OST equals Match Register 1
				   0xFF, // PPID 28 OST equals Match Register 2
				   0xFF, // PPID 29 OST equals Match Register 3
				   0xFF, // PPID 30 RTC One HZ TIC
				   0xFF, // PPID 31 RTC equals Alarm
				   0xFF, // PPID 32
				   0x09, // PPID 33 Quick Capture Interface
				   0xFF, // PPID 34
				   0xFF, // PPID 35
				   0xFF, // PPID 36
				   0xFF, // PPID 37
				   0xFF, // PPID 38
				   0xFF  // PPID 39
};

#endif //_HARDWARE_ONCE
