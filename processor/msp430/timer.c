/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file timer.c
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @brief HAL Layer for System Timer on MSP430
 * @note System timer DOES NOT USE a dedicated hardware timer
 */

#include <sos.h>
#include <sos_timer.h>
#include "timer.h"
#include <led.h>


uint16_t _timerb_prev_tbr;
uint16_t _timerb_interval; // This stores interval in (ACLK/32) Clock units

/**
 * @note June 22,2006 Ram - I am shifting the counter value by 6 as opposed to 5.
 * @todo June 22,2006 Ram - Resolve the timer jitter issue. Still very bad
 */


/** 
 * @brief timer hardware routine
 */
void timer_hardware_init(uint8_t interval){    
  HAS_CRITICAL_SECTION;
  
  ENTER_CRITICAL_SECTION();
  {
    _timerb_prev_tbr = timerb_reliable_read();
	//	TBCCR3 = ((uint16_t)(interval << 6)) + _timerb_prev_tbr;
	TBCCR3 = (uint16_t)(((uint16_t)(interval << 5)) + _timerb_prev_tbr);
	//  TBCCR3 = ((uint16_t)(interval)) + _timerb_prev_tbr;
    _timerb_interval = (uint16_t)interval;
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
  }
  LEAVE_CRITICAL_SECTION();
  // Call the SOS timer init function
  timer_init();
}   

void timer_setInterval(int32_t val)
{
  uint8_t interval;

  if(val > 250) {interval = 250;}
  else if(val <= 2) {interval = 2;}
  else {interval = (uint8_t)val - 1;}
  /** @note 
   * Shift by 5 since running from undivided ACLK at 32,768 Hz
   * Other peripherals cannot use RTC if ACLK is divided.
   * Cannot reset the hardware counter as the timer is not dedicated
   */
  _timerb_prev_tbr = timerb_reliable_read();
  //  TBCCR3 = (uint16_t)(((uint16_t)(interval << 6)) + _timerb_prev_tbr);
  TBCCR3 = (uint16_t)(((uint16_t)(interval << 5)) + _timerb_prev_tbr);
  // TBCCR3 = ((uint16_t)(interval)) + _timerb_prev_tbr;
  _timerb_interval = (uint16_t)interval;
}

