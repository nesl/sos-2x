/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA,
 * 94704.  Attention:  Intel License Inquiry.
 */
/*
 *
 * Authors:   Alec Woo, David Gay, Philip Levis
 * Date last modified:  6/25/02
 *
 */

/**
 * @author Alec Woo
 * @author David Gay
 * @author Philip Levis
 */

/**
 * @author Ram Kumar
 * @brief Porting to SOS
 **/

#ifndef _MICASB_H
#define _MICASB_H


/**
 * @brief sensor type used in ker_sensor_read
 */
#define PHOTO               0
#define TEMPERATURE         1
#define MIC                 2
#define ACCELX              3
#define ACCELY              4
#define MAGX                5
#define MAGY                6 
//#define MAX_SENSOR_TYPE     7 // For MicaSB

enum {
  TOSH_ACTUAL_PHOTO_PORT = 1,
  TOSH_ACTUAL_TEMP_PORT = 1,
  TOSH_ACTUAL_MIC_PORT = 2,
  TOSH_ACTUAL_ACCEL_X_PORT = 3,
  TOSH_ACTUAL_ACCEL_Y_PORT = 4,
  TOSH_ACTUAL_MAG_X_PORT = 6,
  TOSH_ACTUAL_MAG_Y_PORT  = 5
};

enum {
  TOS_ADC_PHOTO_PORT = 1,
  TOS_ADC_TEMP_PORT = 2,
  TOS_ADC_MIC_PORT = 3,
  TOS_ADC_ACCEL_X_PORT = 4,
  TOS_ADC_ACCEL_Y_PORT = 5,
  TOS_ADC_MAG_X_PORT = 6,
  // TOS_ADC_VOLTAGE_PORT = 7,  defined this in hardware.h
  TOS_ADC_MAG_Y_PORT = 8,
};

enum {
  TOS_MAG_POT_ADDR = 0,
  TOS_MIC_POT_ADDR = 1
};

/* void sensor_board_init(); */
/* void sensor_read_fired(); */

#ifndef _MODULE_
int8_t magsensor_init(void);
#endif


#endif // _MICASB_H

