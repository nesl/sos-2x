/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver of the ADS7828
 * 
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * \author Mike Buchanan
 * \date 07-2006
 */

#ifndef _DIGITAL_ADC_H_
#define _DIGITAL_ADC_H_

/**
 * These flags are which power pins to turn on. . . we think
 */

#define THERMISTOR 0x04
#define HUMIREL 0x28

/** 
 * Constant used to mark the start point of message types HANDLED by the
 * ADS7828
 */
#define ADS7828_MSG_IN (MOD_MSG_START + 20) //The 20 is arbitrary.
#define ADS_GET_DATA (ADS7828_MSG_IN + 0)
#define ADS_GET_FROM (ADS7828_MSG_IN + 1)
#define ADS_POWER_ON (ADS7828_MSG_IN + 2)

/** 
 * Constant used to mark the start point of message types PROVIDED by the
 * ADS7828
 */
#define ADS7828_MSG_OUT (ADS7828_MSG_IN + 3)
#define ADS_DATA (ADS7828_MSG_OUT + 0)

/** 
 * Constant used to mark the end of the messages associated with the ADS7828
 */
#define ADS7828_MSG_END (ADS7828_MSG_OUT + 1)


#ifndef _MODULE_
extern mod_header_ptr ads7828_get_header();
#endif

#endif // _DIGITAL_ADC_H_


