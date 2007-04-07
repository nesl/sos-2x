/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file timer.c
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @brief HAL Layer for System Timer on MSP430
 * @note System timer DOES NOT USE a dedicated hardware timer
 */

#include <io.h>
#include <signal.h>
#include <timerb_hal.h>

#include <sos.h>
#include <sos_timer.h>
#include "timer.h"
#include <led.h>


#define TIMERB_ASYNC_SHIFT_VAL       5

uint16_t _timerb_prev_tbr;
uint8_t _timerb_interval; 
static volatile uint16_t currentTime;

/**
 * @note June 22,2006 Ram - I am shifting the counter value by 6 as opposed to 5.
 * @todo June 22,2006 Ram - Resolve the timer jitter issue. Still very bad
 */
void timerb_hal_init()
{
  // TBCTL
  // .TBCLGRP =  0; each TBCL group latched independently
  // .CNTL    =  0; 16-bit counter
  // .TBSSEL  =  1; source ACLK
  // .ID      =  0; input divisor of 1
  // .MC      = 10; Continuous mode
  // .TBCLR   =  0; reset timer B
  // .TBIE    =  1; enable timer B interrupts
  //TBCTL = (TBSSEL0 | MC1 | TBIE) & (~TBCLR);

  TBCTL = (TBSSEL0 | ID_0 | MC_2 | TBIE) & (~TBCLR);

	////////////////////////////////////////////
    // TBCCTL3
    // .CMx     =  00; No Capture
    // .CCISx   =  00
    // .SCS     =   0
    // .CLLDx   =  00; Load TBCL3 on write to TBCCR3
    // .CAP     =   0; Compare Mode
    // .OutModx = 000
    // .CCIE    =   1; Compare/Capture Interrupt Enable
    // .CCI     =  IN; Capture Input
    // .OUT     =   0
    // .CV      =  IN
    // .CCIFG   =  IN; Compare/Capture interrupt flag  
    TBCCTL3 = CCIE;
	
  return;
}


/** 
 * @brief timer hardware routine
 */
void timer_hardware_init(uint8_t interval)
{
  timer_setInterval(interval);    
  // Call the SOS timer init function
  timer_init();
}   

void timer_setInterval(int32_t val)
{
  HAS_CRITICAL_SECTION;
  uint16_t interval;


  interval = val << TIMERB_ASYNC_SHIFT_VAL;
  
  ENTER_CRITICAL_SECTION();
  _timerb_prev_tbr = timerb_reliable_read();
  
  TBCCR3 = interval + _timerb_prev_tbr;
  
  _timerb_interval = (uint8_t)val;
  LEAVE_CRITICAL_SECTION();
}


uint8_t timer_hardware_get_counter() 
{
	HAS_CRITICAL_SECTION;
	uint16_t temp;
	ENTER_CRITICAL_SECTION();
	temp = timerb_reliable_read() - _timerb_prev_tbr;
	LEAVE_CRITICAL_SECTION();
	return (uint8_t)( temp >> TIMERB_ASYNC_SHIFT_VAL );
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
		_timerb_prev_tbr = timerb_reliable_read();
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
  case 14: // Overflow 
	currentTime++;
    break;
  default:
    break;
  }
}

// Systime implementation
union time_u
{
    struct
    {
        uint16_t low;
        uint16_t high;
    };
    uint32_t full;
};

uint16_t ker_systime16L()
{
  return timerb_reliable_read();
}

uint16_t ker_systime16H()
{ 
  return currentTime;
}

uint32_t ker_systime32()
{ 
	register union time_u time;
	HAS_CRITICAL_SECTION;
    
    ENTER_CRITICAL_SECTION();
    {   
        time.low = timerb_reliable_read();
        time.high = currentTime;
        
        // maybe there was a pending interrupt
        if( TBCTL & TBIFG )
            ++time.high;
    }
    LEAVE_CRITICAL_SECTION();   
  
	return time.full;
}




