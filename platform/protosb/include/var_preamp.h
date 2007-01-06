/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
      
#ifndef _VAR_PREAMP_H_
#define _VAR_PREAMP_H_

#define CMD_PENDING 0x01

#define GAIN_0			0x00
#define GAIN_1			0x01
#define GAIN_2			0x02
#define GAIN_4			0x03
#define GAIN_8			0x04
#define GAIN_16			0x05
#define GAIN_32			0x06
#define GAIN_64			0x07
#define GAIN_128		0x08
#define GAIN_256		0x09
#define GAIN_512		0x0a
#define GAIN_1024		0x0b
#define GAIN_2048		0x0c
#define GAIN_4096		0x0f // or 0x0d, 0x0e

#define var_preamp_on() CLR_AMP_SHDN()
#define var_preamp_off() SET_AMP_SHDN()

/** @brief Slave interupt handler */
extern int8_t var_preamp_init();
extern int8_t var_preamp_hardware_init();

#ifndef _MODULE_
#include <sos.h>
extern int8_t ker_var_preamp_setGain(uint8_t calling_id, uint8_t gain);
#endif

#endif // _VAR_PREAMP_H_

