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
 *
 */

/**
 * @author David Lee
 * @author Roy Shea
 */

#ifndef _ONE_WIRE_H
#define _ONE_WIRE_H

#include <sos_types.h>

/**
 * @brief init function for servos
 */
void one_wire_init();

/**
 * Write data on the one wire bus with no read expected 
 * 
 * \param data The packet that will be written on the one-wire bus
 * 
 * \param size The packet size (number of bytes) to write on the one-wire bus
 * 
 * \return SOS_OK if successful and -EBUSY if unsuccessful
 */
int8_t ker_one_wire_write(uint8_t* command_data, uint8_t command_size,
        uint8_t* write_data, uint8_t write_size);


/**
 * Write data on the one wire bus followed by a Read on the bus  
 * 
 * \param write_data The packet that will be written on the one-wire bus
 * 
 * \param write_size The packet size (number of bytes) to write on the one-wire bus
 * 
 * \param read_data The packet read off of the one-wire bus
 * 
 * \param read_size The packet size (number of bytes) to read on the one-wire bus 
 * 
 * \return SOS_OK if successful and -EBUSY if unsuccessful
 */
int8_t ker_one_wire_read(uint8_t* command_data, 
        uint8_t command_size, 
        uint8_t *read_data, 
        uint8_t read_size);

#endif //_ONE_WIRE_H

