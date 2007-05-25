/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the SHT15
 * 
 * This module currently provides support for measuring
 * temperature and humidity from the SHT15 chip.
 * 
 * \author Kapy Kangombe, John Hicks, James Segedy {jsegedy@gmail.com}
 * \date 07-2005
 * Ported driver from TinyOS to SOS
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * Ported driver to current version of SOS
 */

#ifndef _SHT15_H_
#define _SHT15_H_


/**
 * ID of the SHT15 module
 */
#define SHT15_ID (DFLT_APP_ID1)


/** 
 * Constant used to mark the start point of message types HANDLED by the
 * SHT15
 */
#define SHT15_MSG_IN (MOD_MSG_START + 0)

/** 
 * Constant used to mark the start point of message types PROVIDED by the
 * SHT15
 */
#define SHT15_MSG_OUT (SHT15_MSG_IN + 2)

/** 
 * Constant used to mark the end of the messages associated with the SHT15
 */
#define SHT15_MSG_END (SHT15_MSG_OUT + 2)


/**
 * SHT15 specific mesage types HANDLED by the SHT15 from other modules
 */
enum
{
    SHT15_GET_TEMPERATURE = (SHT15_MSG_IN + 0),
    SHT15_GET_HUMIDITY = (SHT15_MSG_IN + 1),
};


/**
 * DS2438 specific mesage types SENT by the DS2438 to other modules
 */
enum
{
    SHT15_TEMPERATURE = (SHT15_MSG_OUT + 0),
    SHT15_HUMIDITY = (SHT15_MSG_OUT + 1),
};


#endif // _SHT15_H_

