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

#ifndef _SOS_CC1K_H
#define _SOS_CC1K_H

/**
 * @brief CC1K timing helper functions
 */
static inline void TOSH_uwait(uint16_t _u_sec) {        
    while (_u_sec > 0) {            
	/* XXX SIX nop will give 8 cycles, which is 1088 ns */
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      _u_sec--;                     
    }                               
}

/**
 * @brief CC1K radio related low-level functions
 */
extern void cc1k_init();
extern void cc1k_write(uint8_t addr, uint8_t data);
extern uint8_t cc1k_read(uint8_t addr);
extern bool cc1k_getLock();

/**
 * @brief CC1K control related functions
 */
extern void cc1k_cnt_TunePreset(uint8_t freq);
extern uint32_t cc1k_cnt_TuneManual(uint32_t DesiredFreq);
extern void cc1k_cnt_TxMode();
extern void cc1k_cnt_RxMode();
extern void cc1k_cnt_BIASOff();
extern void cc1k_cnt_BIASOn();
extern void cc1k_cnt_stop();
extern void cc1k_cnt_start();
extern void cc1k_cnt_init();

extern void cc1k_cnt_SetRFPower(uint8_t power);
extern uint8_t cc1k_cnt_GetRFPower();
extern void cc1k_cnt_SelectLock(uint8_t Value);
extern uint8_t cc1k_cnt_GetLock();
extern bool cc1k_cnt_GetLOStatus();

/**
 * @brief CC1k Radio Functions
 */
extern int8_t cc1k_radio_init();
extern int8_t cc1k_radio_start();
extern int8_t cc1k_radio_stop();
extern int8_t cc1k_radio_spi_interrupt(uint8_t data_in);
extern void RSSIADC_dataReady(uint16_t data);
extern int8_t radio_set_timestamp(bool on);

/**
 * @brief SOS Radio API Interface
 */
extern int8_t radio_init();
extern void radio_msg_alloc(Message *e);
extern void ker_radio_ack_enable();
extern void ker_radio_ack_disable();

#endif // _SOS_CC1K_H

