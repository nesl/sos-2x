/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @file timerb_hal.h
 * @brief  TIMERB HAL for MSP430
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @note Ported from TinyOS
 */

#ifndef _TIMERB_HAL_H_
#define _TIMERB_HAL_H_

static inline uint16_t timerb_reliable_read()
{
  uint16_t val1, val2, val3;
  //! Must do a majority vote to safely read async. timer   
  while(1){
    val1 = TBR;
    val2 = TBR;
    val3 = TBR;
    if(val1 == val2) return val1;
    else if(val2 == val3) return val2;
    else if(val1 == val3) return val1;
  }
}


/**
 * @brief Initialize TimerB HAL
 */
void timerb_hal_init();

#endif//_TIMERB_HAL_H_

