/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
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
 *
 */

#ifndef _ADC_PROC_HW_H
#define _ADC_PROC_HW_H

#include <sos_types.h>


#define ADC_PROC_REF_AREF     (0x00)
#define ADC_PROC_REF_AVCC     _BV(REFS0)
#define ADC_PROC_REF_RSRVD    _BV(REFS1)
#define ADC_PROC_REF_INTERNAL (_BV(REFS1)|_BV(REFS0))


/**
 * single ended channels
 * 
 * (MUX4 | MUX3 | MUX2 | MUX1 | MUX0))
 * MUX[4-0]   +IN
 * 0 0 0 0 0  CH0
 * 0 0 0 0 1  CH1
 * 0 0 0 1 0  CH2
 * 0 0 0 1 1  CH3
 * 0 0 1 0 0  CH4
 * 0 0 1 0 1  CH5
 * 0 0 1 1 0  CH6
 * 0 0 1 1 1  CH7
 */
#define ADC_PROC_CH0 0x00
#define ADC_PROC_CH1 0x01
#define ADC_PROC_CH2 0x02
#define ADC_PROC_CH3 0x03
#define ADC_PROC_CH4 0x04
#define ADC_PROC_CH5 0x05
#define ADC_PROC_CH6 0x06
#define ADC_PROC_CH7 0x07


/**
 * differential channels
 * 
 * +IN -IN gain
 * 
 * CH0 CH0 10x
 * CH1 CH0 10x
 * CH0 CH0 200x
 * CH1 CH0 200x
 * 
 * CH2 CH2 10x
 * CH3 CH2 10x
 * CH2 CH2 200x
 * CH3 CH2 200x
 * 
 * CH0 CH1 1x
 * CH1 CH1 1x
 * CH2 CH1 1x
 * CH3 CH1 1x
 * CH4 CH1 1x
 * CH5 CH1 1x
 * CH6 CH1 1x
 * CH7 CH1 1x
 * 
 * CH0 CH2 1x
 * CH1 CH2 1x
 * CH2 CH2 1x
 * CH3 CH2 1x
 * CH4 CH2 1x
 * CH5 CH2 1x
 */
#define ADC_PROC_DIFF_CH_0_0_10x  0x08
#define ADC_PROC_DIFF_CH_1_0_10x  0x09
#define ADC_PROC_DIFF_CH_0_0_200x 0x0A
#define ADC_PROC_DIFF_CH_1_0_200x 0x0B

#define ADC_PROC_DIFF_CH_2_2_10x  0x0C
#define ADC_PROC_DIFF_CH_3_2_10x  0x0D
#define ADC_PROC_DIFF_CH_2_2_200x 0x0E
#define ADC_PROC_DIFF_CH_3_2_200x 0x0F

#define ADC_PROC_DIFF_CH_0_1      0x10
#define ADC_PROC_DIFF_CH_1_1      0x11
#define ADC_PROC_DIFF_CH_2_1      0x12
#define ADC_PROC_DIFF_CH_3_1      0x13
#define ADC_PROC_DIFF_CH_4_1      0x14
#define ADC_PROC_DIFF_CH_5_1      0x15
#define ADC_PROC_DIFF_CH_6_1      0x16
#define ADC_PROC_DIFF_CH_7_1      0x17

#define ADC_PROC_DIFF_CH_0_2      0x18
#define ADC_PROC_DIFF_CH_1_2      0x19
#define ADC_PROC_DIFF_CH_2_2      0x1A
#define ADC_PROC_DIFF_CH_3_2      0x1B
#define ADC_PROC_DIFF_CH_4_2      0x1C
#define ADC_PROC_DIFF_CH_5_2      0x1D


/**
 * Radio driver channels
 */
#ifdef MICA2_PLATFORM
#define MICA2_CC_RSSI_SID         0x00
#define MICA2_CC_RSSI_HW_PORT     ADC_PROC_CH0
#endif



/**
 * calibration channels
 */
#define ADC_PROC_BANDGAP          0x1E
#define ADC_PROC_GND              0x1F

#define ADC_PROC_HW_CH_MAX        0x20

#define ADC_PROC_HW_NULL_PORT     0xFF

/**
 * adc prescaler
 * 
 * need to bitshift value to location in register
 * (_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0))
 * 
 * Note: NULL is actually CLK_2
 */
#define ADC_PROC_CLK_NULL  (0x00)
#define ADC_PROC_CLK_2    _BV(ADPS0) // 1
#define ADC_PROC_CLK_4    _BV(ADPS1) // 2
#define ADC_PROC_CLK_8    (_BV(ADPS1)|_BV(ADPS0)) // 3
#define ADC_PROC_CLK_16   _BV(ADPS2) // 4
#define ADC_PROC_CLK_32   (_BV(ADPS2)|_BV(ADPS0)) // 5
#define ADC_PROC_CLK_64   (_BV(ADPS2)|_BV(ADPS1)) // 6
#define ADC_PROC_CLK_128  (_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0)) // 7

#define ADC_PROC_CLK_MSK  (_BV(ADPS2)|_BV(ADPS1)|_BV(ADPS0)) // 7

#define adc_proc_interrupt() SIGNAL(SIG_ADC)

/** 
 * @brief init function for ADC
 */
extern int8_t adc_proc_hardware_init();

#endif // _ADC_PROC_HW_H 

