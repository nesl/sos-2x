/**
 * @brief    device related routines 
 * @author   Naim Busek	(naim@lecs.cs.ucla.edu)
 * @version  0.1
 *
 */
#ifndef _PID_PLAT_H
#define _PID_PLAT_H
#include <sos_types.h>
#include <pid_proc.h>

/**
 * @brief device pid list
 */
// DEV_MOD_MIN_PID = 0x40
#define PWR_MGMT_PID	(DEV_MOD_MIN_PID + 1)
#define SPI_PID 	(DEV_MOD_MIN_PID + 2)
#define ADG715_MUX_PID  (DEV_MOD_MIN_PID + 3)   //!< pid for protosb mux
#define KER_UART_PID    (DEV_MOD_MIN_PID + 4)
#define KER_I2C_PID     (DEV_MOD_MIN_PID + 5)
#define LTC6915_AMP_PID (DEV_MOD_MIN_PID + 6)   //!< pid for protosb variable gain amplifier handler
#define ADS8341_ADC_PID (DEV_MOD_MIN_PID + 7)   //!< pid for protosb adc daemon
#define KER_I2C_MGR_PID (DEV_MOD_MIN_PID + 8)

#define PLAT_MAX_PID    (PROC_MAX_PID + 8)

#endif

