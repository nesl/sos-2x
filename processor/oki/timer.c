/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
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

#include <hardware.h>
#include <sos_timer.h>
#include "timer.h"
#include <hardware_proc.h>
//#include "ML674000.h"
//#include "irq.h"

/* constants */
//#define MHz     (1000000L)
//#define TMRCYC  (30)      /* interval of TIMER interrupt (ms) */
//#define CCLK    (56*MHz)    /* frequency of CCLK */
//#define VALUE_OF_TMRLR      /* reload value of timer */
 //               ((0x10000L*(16*1000)-((TMRCYC)*(CCLK)))/(16*1000))
static void set_timer(unsigned long timer_value);
static void timer0_interrupt_handler();

void enable_timer(void)
{
	set_wbit(ILC, ILC_ILC16);
}

void disable_timer(void)
{
	clr_wbit(ILC, ILC_ILC16);
}

/**
 * @brief timer hardware routine
 */
void timer_hardware_init(uint8_t interval, uint8_t scale){

    HAS_CRITICAL_SECTION;
	unsigned long timer_value;
	timer_value = ((0x10000L*(16*1000)-((interval)*(CCLK)))/(16*1000));
    ENTER_CRITICAL_SECTION();
	IRQ_HANDLER_TABLE[INT_TIMER0] = timer0_interrupt_handler;
	set_wbit(ILC, ILC_ILC16 & ILC_INT_LV1); /* interrupt level of
                       ||            ||          IRQ number 16 (timer0)  is set as 1
                   IRQ number  interrupt level  */

    //! Initialize and start hardware timer
	set_timer(timer_value);

    LEAVE_CRITICAL_SECTION();
	// enable timer by writing '1' in TMEN[0] register
    put_wvalue(TMEN, 0x01);

    //! Call the SOS timer init function
    timer_init();
}

/**
 * @brief real Timer handler
 */
static void timer0_interrupt_handler()
{
	_timer_interrupt();
	put_wvalue(TIMESTAT0, 0x01);   // implicitly clear the STATUS of the timer0
								   // so that timer interrupts can be re-enabled
}

/******************************************************************************
 * Timer0_setInterval													 	  *
 * Description: alters the interval for an instance of the time				  *
 * Parameters: 1) interval - specifies running time of timer 				  *
 * Returns a character symbolizing PALOS_TASK_DONE							  *
 ******************************************************************************/

void timer_setInterval(int32_t interval)
{
	// convert interval so it accurately reflects time in seconds
	unsigned long timer_value;
		
	if(interval > 250) interval = 250;
	timer_value = ((0x10000L*(16*1000)-((interval)*(CCLK)))/(16*1000));

	// set the timer with the appropriate value

	set_timer(timer_value);
}

/*------------------------ functions about TIMER -------------------------------*/

/****************************************************************************/
/*  Setup of timer                                                          */
/*  Function : set_timer                                                    */
/*      Parameters                                                          */
/*          Input   :   Nothing                                             */
/*          Output  :   Nothing                                             */
/****************************************************************************/
static void set_timer(unsigned long timer_value)
{
	put_wvalue(TIMECNTL0, (get_wvalue(TIMECNTL0) & 0xFFE7));   // disable interrupt and stop timer0
	put_wvalue(TIMECMP1, timer_value);						  // load the new interval in the compare register
	put_wvalue(TIMEBASE0, 0x0000);							  // initialize the time base of the counter
	put_wvalue(TIMECNTL0, (get_wvalue(TIMECNTL0) & 0xFFF7));  // make timer periodic
	put_wvalue(TIMECNTL0, (get_wvalue(TIMECNTL0) | 0x0018));  // enable interrupt and start timer0 as one-shot timer

    return;
}

/****************************************************************************/
/*  Registration of IRQ Handler                                             */
/*  Function : reg_irq_handler                                              */
/*      Parameters                                                          */
/*          Input   :   Nothing                                             */
/*          Output  :   Nothing                                             */
/*  Note : Initialization of IRQ needs to be performed before this process. */
/****************************************************************************/
//void reg_irq_handler(void)
//{
    /***********************************************************
      IRQ handler (timer_handler) is registered into IRQ handler table.
    ***********************************************************/
    //IRQ_HANDLER_TABLE[INT_SYSTEM_TIMER] = (*Timer0_handler);

    /***********************************************************
      setup of ILC0.
      ILC0 sets interrupt level of IRQ number 0-7.
      ILC0_ILR0 corresponds to IRQ number 0.
      ILC0_ILR1 corresponds to IRQ number 1-3.
      ILC0_ILR4 corresponds to IRQ number 4 and 5.
      ILC0_ILR6 corresponds to IRQ number 6 and 7.
    ***********************************************************/
    /* set_wbit(ILC0, ILC0_ILR0 & ILC0_INT_LV1);  interrupt level of
                       ||            ||          IRQ number 0 is set as 1
                   IRQ number  interrupt level  */

    /***********************************************************
      setup of ILC1.
      ILC1 sets interrupt level of IRQ number 8-15.
      ILC1_ILR8 corresponds to IRQ number 8.
      ILC1_ILR9 corresponds to IRQ number 9.
      ILC1_ILR10 corresponds to IRQ number 10.
      ILC1_ILR11 corresponds to IRQ number 11.
      ILC1_ILR12 corresponds to IRQ number 12.
      ILC1_ILR13 corresponds to IRQ number 13.
      ILC1_ILR14 corresponds to IRQ number 14.
      ILC1_ILR15 corresponds to IRQ number 15.
    ***********************************************************/
    /* in this sample ILC1 isn't used */

    /***********************************************************
      setup of ILC.
      ILC sets interrupt level of IRQ number 16-31.
      ILC_ILR16 corresponds to IRQ number 16 and 17.
      ILC_ILR18 corresponds to IRQ number 18 and 19.
      ILC_ILR20 corresponds to IRQ number 20 and 21.
      ILC_ILR22 corresponds to IRQ number 22 and 23.
      ILC_ILR24 corresponds to IRQ number 24 and 25.
      ILC_ILR26 corresponds to IRQ number 26 and 27.
      ILC_ILR28 corresponds to IRQ number 28 and 29.
      ILC_ILR30 corresponds to IRQ number 30 and 31.
    ***********************************************************/
    /* in this sample ILC isn't used */

    /***********************************************************
      setup of IDM.
      IDM sets IRQ detection mode and interrupt level of IRQ number 16-31.
      IDM_IDM16 and IDM_IDMP16 correspond to IRQ number 16 and 17.
      IDM_IDM18 and IDM_IDMP18 correspond to IRQ number 18 and 19.
      IDM_IDM20 and IDM_IDMP20 correspond to IRQ number 20 and 21.
      IDM_IDM22 and IDM_IDMP22 correspond to IRQ number 22 and 23.
      IDM_IDM24 and IDM_IDMP24 correspond to IRQ number 24 and 25.
      IDM_IDM26 and IDM_IDMP26 correspond to IRQ number 26 and 27.
      IDM_IDM28 and IDM_IDMP28 correspond to IRQ number 28 and 29.
      IDM_IDM30 and IDM_IDMP30 correspond to IRQ number 30 and 31.
    ***********************************************************/
    /* in this sample IDM isn't used */

    //return;
//}


