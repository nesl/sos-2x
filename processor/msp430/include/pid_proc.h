/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief    MSP430 Processor Related PID 
 * @author   Ram Kumar {ram@ee.ucla.edu}
 * $Id: pid_proc.h,v 1.1 2006/02/12 22:25:27 ram Exp $
 */
//--------------------------------------------------------
// MSP430 PROCESSOR RELATED PIDS
//--------------------------------------------------------
#ifndef _PID_PROC_H
#define _PID_PROC_H
#include <sos_types.h>

/**
 * @brief Processor PID List
 */
enum{
  /* 65 */  UART_PID = (DEV_MOD_MIN_PID + 1), //! UART Driver
  /* 66 */  ADC_PROC_PID = (DEV_MOD_MIN_PID + 2), //! ADC Driver
  /* 67 */  ADC_DRIVER_PID = (DEV_MOD_MIN_PID + 3), //! ADC Driver (New API)
  /* 68 */  PROC_INTERRUPT_PID = (DEV_MOD_MIN_PID + 4), //! Interrupt controller
};
//Note :- Please update the PROC_MAX_PID

#define PROC_MAX_PID        (DEV_MOD_MIN_PID + 4)

#endif  // _PID_PROC_H
