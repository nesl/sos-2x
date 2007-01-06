/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * @brief    device related routines 
 * @author   Simon Han (simonhan@ee.ucla.edu)
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
/*
enum
  {
	PWR_MGMT_PID         = (PROC_MAX_PID + 1),
	EXFLASH_PID          = (PROC_MAX_PID + 2),
	TIMER3_PID           = (PROC_MAX_PID + 3),
	KER_UART_PID         = (PROC_MAX_PID + 4),
	KER_I2C_PID          = (PROC_MAX_PID + 5),
	SPI_PID              = (PROC_MAX_PID + 6),
	KER_I2C_MGR_PID      = (PROC_MAX_PID + 7), 
	ADCM1700_CONTROL_PID   			= (PROC_MAX_PID + 8), 
	ADCM1700_COMM_PID   				= (PROC_MAX_PID + 9),
	CPLD_PID      						= (PROC_MAX_PID + 10),
	ADCM1700_WINDOW_SIZE_PID   	        = (PROC_MAX_PID + 11),
	ADCM1700_RUN_PID             			= (PROC_MAX_PID + 12), 
	ADCM1700_STAT_PID             			= (PROC_MAX_PID + 13), 
	ADCM1700_PATTERN_PID        			= (PROC_MAX_PID + 14), 
	ADCM1700_PATCH_PID        			= (PROC_MAX_PID + 15), 
	ADCM1700_FORMAT_PID        			= (PROC_MAX_PID + 16), 
	ADCM1700_EXPOSURE_PID        		= (PROC_MAX_PID + 17), 
	SERIAL_DUMP_PID		        		= (PROC_MAX_PID + 18), 
	RADIO_DUMP_PID		        		= (PROC_MAX_PID + 19), 
	MATRIX_IMAGE_PID		        		= (PROC_MAX_PID + 20), 
	MATRIX_ARITHMETICS_PID		        	= (PROC_MAX_PID + 21), 
	IMG_BACKGROUND_PID		        	= (PROC_MAX_PID + 22), 
	  
  };
  */

/*
 * Communication PID's should match with both
 * mica2 and micaz so that cyclops can interact
 * with any one of them.
 */
enum
  {
	PWR_MGMT_PID         = (PROC_MAX_PID + 1),
	EXFLASH_PID          = (PROC_MAX_PID + 2),
	TIMER3_PID           = (PROC_MAX_PID + 3),
	SPI_PID              = (PROC_MAX_PID + 4),
	KER_UART_PID         = (PROC_MAX_PID + 5),
	KER_I2C_PID          = (PROC_MAX_PID + 6),
	KER_I2C_MGR_PID      = (PROC_MAX_PID + 7), 
	ADCM1700_CONTROL_PID   			= (PROC_MAX_PID + 8), 
	ADCM1700_COMM_PID   				= (PROC_MAX_PID + 9),
	CPLD_PID      						= (PROC_MAX_PID + 10),
	ADCM1700_WINDOW_SIZE_PID   	        = (PROC_MAX_PID + 11),
	ADCM1700_RUN_PID             			= (PROC_MAX_PID + 12), 
	ADCM1700_STAT_PID             			= (PROC_MAX_PID + 13), 
	ADCM1700_PATTERN_PID        			= (PROC_MAX_PID + 14), 
	ADCM1700_PATCH_PID        			= (PROC_MAX_PID + 15), 
	ADCM1700_FORMAT_PID        			= (PROC_MAX_PID + 16), 
	ADCM1700_EXPOSURE_PID        		= (PROC_MAX_PID + 17), 
	SERIAL_DUMP_PID		        		= (PROC_MAX_PID + 18), 
	RADIO_DUMP_PID		        		= (PROC_MAX_PID + 19), 
	MATRIX_IMAGE_PID		        		= (PROC_MAX_PID + 20), 
	MATRIX_ARITHMETICS_PID		        	= (PROC_MAX_PID + 21), 
	IMG_BACKGROUND_PID		        	= (PROC_MAX_PID + 22), 
	  
  };
// Note:- Update the PLAT_MAX_PID

#define PLAT_MAX_PID         (PROC_MAX_PID + 22)
//#define PLAT_MAX_PID         (PROC_MAX_PID + 7)

// XXX what is the max value?
#endif

