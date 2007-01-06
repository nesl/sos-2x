/**
 * @brief    device related routines 
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  0.1
 *
 */
#ifndef _PID_PROC_H
#define _PID_PROC_H
#include <sos_types.h>
/**
 * @brief device pid list
 */
#define PWR_MGMT_PID         (DEV_MOD_MIN_PID + 1)
#define MICASB_PID           (DEV_MOD_MIN_PID + 2)
#define UART_PID             (DEV_MOD_MIN_PID + 3)
#define I2C_PID              (DEV_MOD_MIN_PID + 4)
#define EXFLASH_PID          (DEV_MOD_MIN_PID + 5)

#define PROC_MAX_PID         (DEV_MOD_MIN_PID + 5)

#endif

