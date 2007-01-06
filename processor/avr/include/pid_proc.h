/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief    AVR Processor Related PID 
 * @author   Naim Busek <ndbusek@gmail.com>
 * $Id: pid_proc.h,v 1.6 2006/08/30 22:24:51 martinm Exp $
 */
//--------------------------------------------------------
// AVR ATMEGA128L PROCESSOR RELATED PIDS
//--------------------------------------------------------

#ifndef _PID_PROC_H
#define _PID_PROC_H
#include <sos_types.h>

/**
 * @brief Processor PID List
 */
enum{
  /* 65 */  I2C_PID  = (DEV_MOD_MIN_PID + 1), //! I2C Driver
  /* 66 */  UART_PID = (DEV_MOD_MIN_PID + 2), //! UART Driver
  /* 67 */  ADC_PROC_PID = (DEV_MOD_MIN_PID + 3), //! ADC Driver
};
//Note :- Please update the PROC_MAX_PID

#define PROC_MAX_PID        (DEV_MOD_MIN_PID + 3)

#endif  // _PID_PROC_H

