/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @file sos_error_types.h
 * @brief Error Types in the SOS Kernel
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _SOS_ERROR_TYPES_H_
#define _SOS_ERROR_TYPES_H_

enum {
    READ_ERROR = 0,       //! bus read
	SEND_ERROR,           //! bus send failed
    SENSOR_ERROR,         //! sensor failure (other than read/write i.e. sensor removed)
    MALLOC_ERROR,         //! malloc failed for reply msg
  };

#endif//_SOS_ERROR_TYPES_H_
