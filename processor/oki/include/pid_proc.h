/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief    AVR Processor Related PID 
 * @author   Naim Busek <ndbusek@gmail.com>
 * $Id: pid_proc.h,v 1.1 2006/02/17 04:06:48 simonhan Exp $
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
};
//Note :- Please update the PROC_MAX_PID

#define PROC_MAX_PID        (DEV_MOD_MIN_PID + 2)

#endif  // _PID_PROC_H

