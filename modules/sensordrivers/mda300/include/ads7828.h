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

#ifndef _ADS7828_H_
#define _ADS7828_H_

/**
 * These flags are which power pins to turn on. . . we think
 */

#define THERMISTOR 0x04
#define HUMIREL 0x28

// All the sensors we export
enum {
	// Single ended channels referenced to COM
	ADS7828_CH0_SID=1,
	ADS7828_CH1_SID,
	ADS7828_CH2_SID,
	ADS7828_CH3_SID,
	ADS7828_CH4_SID,
	ADS7828_CH5_SID,
	ADS7828_CH6_SID,
	// Note that channel 7 isn't connected on the MDA300
	ADS7828_CH7_SID,

// We don't actually export these because you can only have 10 sensors.
	// Differential channels -- one direction
	ADS7828_DIFF_CH_0_1_SID,
	ADS7828_DIFF_CH_2_3_SID,
	ADS7828_DIFF_CH_4_5_SID,
	// Not very useful, as 7 isn't attached
	ADS7828_DIFF_CH_6_7_SID,
	// Differential channels -- opposite direction
	ADS7828_DIFF_CH_1_0_SID,
	ADS7828_DIFF_CH_3_2_SID,
	ADS7828_DIFF_CH_5_4_SID,
	// Again, kind of useless
	ADS7828_DIFF_CH_7_6_SID,
};

#ifndef _MODULE_
extern mod_header_ptr ads7828_get_header();
#endif

#endif // _ADS7828_H_


