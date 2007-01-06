/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @file timerb_hal.c
 * @brief  TIMERB HAL for MSP430
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @note Ported from TinyOS
 */

#include <io.h>
#include <signal.h>
#include <timerb_hal.h>
#include <timer.h>

/*
#define LED_DEBUG
#include <led_dbg.h>
#include <pin_defs.h>
#include <pin_map.h>
*/


void timerb_hal_init()
{
  // TBCTL
  // .TBCLGRP =  0; each TBCL group latched independently
  // .CNTL    =  0; 16-bit counter
  // .TBSSEL  =  1; source ACLK
  // .ID      =  0; input divisor of 1
  // .MC      = 10; initially disabled
  // .TBCLR   =  0; reset timer B
  // .TBIE    =  1; enable timer B interrupts
  TBCTL = (TBSSEL0 | MC1 | TBIE) & (~TBCLR);


	/*
  GIO2_UNSET_ALT(); // Disable any alternate functions of the pin
  SET_GIO2_DD_OUT();// Set the data direction to output
	GIO3_UNSET_ALT();// Disable any alternate functions of the pin
	SET_GIO3_DD_OUT();// Set the data direction to output
	*/

	/*
		TBCCR4 = (1 << 5);
		TBCCTL4 = CCIE;
  */
  return;
}


interrupt (TIMERB0_VECTOR) timerb0_interrupt()
{
}

interrupt (TIMERB1_VECTOR) timerb1_interrupt()
{
  int n = TBIV;
  switch (n){
  case  0: 
    break;
  case  2: // SFD Interrupt
    break;
  case  4: 
    break;
  case  6: // System Timer
		timerb_compare3_interrupt();
		/*
		TOGGLE_GIO2();
		TBCCR3 = timerb_reliable_read() + (1 << 5);
		*/
    break;
  case  8: // Jiffy Timer
		/*
		TOGGLE_GIO3();
    TBCCR4 = timerb_reliable_read() + (1 << 6);
    */
    break;
  case 10: 
    break;
  case 12: 
    break;
  case 14:
    break;
  default:
    break;
  }
}

