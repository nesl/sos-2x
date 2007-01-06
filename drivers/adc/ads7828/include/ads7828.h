/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2: tab:2 */

/**
 * @author Naim Busek 
 * @author Mike Buchanan
 */

/**
 * Refer to Texas Instruments ADS7828 Datasheet
 */
#ifndef _ADS7828_H_
#define _ADS7828_H_

/**
 * COMMAND BYTE
 * BIT | 7  | 6  | 5  | 4  | 3   | 2   | 1 | 0 |
 *     | SD | C2 | C1 | C0 | PD1 | PD0 | X | X |
 */
#define ADS7828_SD		0x80 //! single-ended/!differential select
#define ADS7828_A2		0x40 //! channel select [2]
#define ADS7828_A1		0x20 //! channel select [1]
#define ADS7828_A0		0x10 //! channel select [0]
#define ADS7828_PD1		0x08 //! power down mode [1]
#define ADS7828_PD0		0x04 //! power down mode [0]
//								 		0x02 // unused
//								 		0x01 // unused

/**
 * PD1 PD0 DESCRIPTION
 *  0   0  Power-down between ADC conversions
 *  0   1  Internal Reference OFF, ADC ON
 *  1   0  Internal Reference ON,  ADC OFF
 *  1   1  Internal Reference ON,  ADC ON
 */

/** 
 * differential channels
 * 
 * SD = 0
 * A2 A1 A0  +IN   -IN
 *  0  0  0  CH0 - CH1
 *  0  0  1  CH2 - CH3
 *  0  1  0  CH4 - CH5
 *  0  1  1  CH6 - CH7
 *  1  0  0  CH1 - CH0
 *  1  0  1  CH3 - CH2
 *  1  1  0  CH5 - CH4
 *  1  1  1  CH7 - CH6
 */
#define ADS7828_DIFF_CH_0_1  0x00
#define ADS7828_DIFF_CH_2_3  0x10
#define ADS7828_DIFF_CH_4_5  0x20
#define ADS7828_DIFF_CH_6_7  0x30
#define ADS7828_DIFF_CH_1_0  0x40
#define ADS7828_DIFF_CH_3_2  0x50
#define ADS7828_DIFF_CH_5_4  0x60
#define ADS7828_DIFF_CH_7_6  0x70


/**
 * single ended channels
 * all channels referenced to COM
 *
 * SD = 1
 * 
 * A2 A1 A0  +IN
 *  0  0  0  CH0
 *  0  0  1  CH2
 *  0  1  0  CH4
 *  0  1  1  CH6
 *  1  0  0  CH1
 *  1  0  1  CH3
 *  1  1  0  CH5
 *  1  1  1  CH7
 */
#define ADS7828_CH0		0x80
#define ADS7828_CH1 	0xC0
#define ADS7828_CH2 	0x90
#define ADS7828_CH3 	0xD0
#define ADS7828_CH4 	0xA0
#define ADS7828_CH5 	0xE0
#define ADS7828_CH6 	0xB0
#define ADS7828_CH7 	0xF0

#endif // _ADS7828_H_
