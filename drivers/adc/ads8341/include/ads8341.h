/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2: tab:2 */

/**
 * @author Naim Busek 
 */

/**
 * Refer to Texas Instruments ADS8341 Datasheet
 */
#ifndef _ADS8341_H_
#define _ADS8341_H_

#define ADS8341_CS 0
/**
 * COMMAND BYTE
 * BIT | 7 | 6  | 5  | 4  | 3 | 2  |  1  |  0  |
 *     | S | A2 | A1 | A0 | - | SD | PD1 | PD0 |
 */
#define ADS8341_S 		0x80 //! start bit
#define ADS8341_A2		0x40 //! channel select [2]
#define ADS8341_A1		0x20 //! channel select [1]
#define ADS8341_A0		0x10 //! channel select [0]
//								 		0x08 // unused
#define ADS8341_SD		0x04 //! single-ended/!differential select
#define ADS8341_PD1		0x02 //! power down mode [1]
#define ADS8341_PD0		0x01 //! power down mode [0]

/**
 * PD1 PD0 DESCRIPTION
 *  0   0  Power-down between conversions
 *  1   0  Selects internal clock mode
 *  0   1  Reserved for future use
 *  1   1  No power-down, external clock mode
 */

/**
 * single ended channels
 * all channels referenced to COM
 *
 * SD = 1
 * A2 A1 A0  +IN
 *  0  0  1  CH0
 *  1  0  1  CH1
 *  0  1  0  CH2
 *  1  1  0  CH3
 */
#define ADS8341_CH0		0x10
#define ADS8341_CH1 	0x50
#define ADS8341_CH2 	0x20
#define ADS8341_CH3 	0x60

/** 
 * differential channels
 * 
 * SD = 0
 * A2 A1 A0  +IN   -IN
 *  0  0  1  CH0 - CH1
 *  1  0  1  CH1 - CH0
 *  0  1  0  CH2 - CH3
 *  1  1  0  CH3 - CH2
 */
#define ADS8341_DIFF_CH_0_1  0x10
#define ADS8341_DIFF_CH_1_0  0x50
#define ADS8341_DIFF_CH_2_3  0x20
#define ADS8341_DIFF_CH_3_2  0x60

#endif // _ADS8341_H_

