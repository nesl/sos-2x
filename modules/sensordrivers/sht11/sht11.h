/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the SHT11
 * 
 * This module currently provides support for measuring
 * temperature and humidity from the SHT11 chip.
 * 
 * \author Kapy Kangombe, John Hicks, James Segedy {jsegedy@gmail.com}
 * \date 07-2005
 * Ported driver from TinyOS to SOS
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * Ported driver to current version of SOS
 *
 * \author Thomas Schmid (thomas.schmid@ucla.edu)
 * \date 07-2007
 * Ported driver from SHT11 to SHT11
 */

#ifndef _SHT11_H_
#define _SHT11_H_


/**
 * ID of the SHT11 module
 */
#define SHT11_ID (DFLT_APP_ID1)


/** 
 * Constant used to mark the start point of message types HANDLED by the
 * SHT11
 */
#define SHT11_MSG_IN (MOD_MSG_START + 0)

/** 
 * Constant used to mark the start point of message types PROVIDED by the
 * SHT11
 */
#define SHT11_MSG_OUT (SHT11_MSG_IN + 2)

/** 
 * Constant used to mark the end of the messages associated with the SHT11
 */
#define SHT11_MSG_END (SHT11_MSG_OUT + 2)


/**
 * SHT11 specific mesage types HANDLED by the SHT11 from other modules
 */
enum
{
    SHT11_GET_TEMPERATURE = (SHT11_MSG_IN + 0),
    SHT11_GET_HUMIDITY = (SHT11_MSG_IN + 1),
};


/**
 * DS2438 specific mesage types SENT by the DS2438 to other modules
 */
enum
{
    SHT11_TEMPERATURE = (SHT11_MSG_OUT + 0),
    SHT11_HUMIDITY = (SHT11_MSG_OUT + 1),
};


#endif // _SHT11_H_

