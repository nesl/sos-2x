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

#include <one_wire.h>
#include <sys_one_wire.h>

// TEMP
#include <led.h>

enum {
    PRESENCE = 0x00
};

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LOCAL VARIABLES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LOCAL FUNCTIONS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static uint8_t start_comm();
static inline void write_zero();
static inline void write_one();
static void write_byte(uint8_t byte);
static uint8_t read_byte();

// Initialization
uint8_t start_comm()
{
    HAS_CRITICAL_SECTION;
    uint8_t presence;

    // Set one wire pin out and hold low for 480us
    ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION | ONE_WIRE_PIN;
    ONE_WIRE_PORT = ONE_WIRE_PORT & ~ONE_WIRE_PIN;
    //LEAVE_CRITICAL_SECTION();
    ds2438_uwait(480);

    // Set one wire pin in and look for "presence" pulse on one wire pin
    //ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION & ~ONE_WIRE_PIN;
    LEAVE_CRITICAL_SECTION();
    ds2438_uwait(60);
    presence = ONE_WIRE_READ_PIN & ONE_WIRE_PIN;
    ds2438_uwait(240);

    if (presence == PRESENCE) {
        ker_led(LED_GREEN_TOGGLE);
    } else {
        ker_led(LED_RED_TOGGLE);
    }
    
    return presence;
}

// Read byte from one wire interface
uint8_t read_byte()
{
    int8_t i;
    uint8_t byte = 0;
    uint8_t readbit = 0;
    HAS_CRITICAL_SECTION;

    for (i = 0; i < 8; i++)
    {
        // Note that the waits are too short to justify leaving the critical
        // section...
        ENTER_CRITICAL_SECTION();

        // Set one wire pin out and hold low for 1us
        ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION | ONE_WIRE_PIN;
        ONE_WIRE_PORT = ONE_WIRE_PORT & ~ONE_WIRE_PIN;
        ds2438_uwait(1);

        // Set one wire pin in, wait 14us, then read the value
        ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION & ~ONE_WIRE_PIN;
        ds2438_uwait(14);
        readbit = (ONE_WIRE_READ_PIN >> ONE_WIRE_PIN_NUMBER) & 0x01;

        LEAVE_CRITICAL_SECTION();

        // Put the bit in the correct place in byte
        byte = byte | (readbit << i);

        ds2438_uwait(46);
    }
    return byte;
}


// Write a zero to the line
void write_zero()
{
    HAS_CRITICAL_SECTION;

    // Set one wire pin out and write zero
    ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION | ONE_WIRE_PIN;
    ONE_WIRE_PORT = ONE_WIRE_PORT & ~ONE_WIRE_PIN;
    //LEAVE_CRITICAL_SECTION();

    // Hold one wire pin low...
    ds2438_uwait(60);

    // Set one wire pin in
    //ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION & ~ONE_WIRE_PIN;
    LEAVE_CRITICAL_SECTION();

    ds2438_uwait(2);
}


// Write a one to the line
void write_one()
{
    HAS_CRITICAL_SECTION;

    // Set one wire pin out, signal data by pulling line low, then write one
    ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION | ONE_WIRE_PIN;
    ONE_WIRE_PORT = ONE_WIRE_PORT & ~ONE_WIRE_PIN;
    ds2438_uwait(2);
    ONE_WIRE_PORT = ONE_WIRE_PORT | ONE_WIRE_PIN;
    //LEAVE_CRITICAL_SECTION();

    // Hold one wire pin high...
    ds2438_uwait(58);

    // Set one wire pin in
    //ENTER_CRITICAL_SECTION();
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION & ~ONE_WIRE_PIN;
    LEAVE_CRITICAL_SECTION();
    ds2438_uwait(2);
}


// Write an entire byte
void write_byte(uint8_t byte)
{
    int i;
    int bit;

    // Send each bit, LSB first, of byte
    for(i=0; i < 8; i++) {
        bit = byte & 0x1;
        switch(bit)
        {
            case 0:
                write_zero();
                break;
            case 1:
                write_one();
                break;
        }
        byte = byte >> 1;
    }
}


// Initialize hardware
void one_wire_init()
{
    // Set one wire pin in
    ONE_WIRE_DIRECTION = ONE_WIRE_DIRECTION & ~ONE_WIRE_PIN;
    ONE_WIRE_PORT = ONE_WIRE_PORT & ~ONE_WIRE_PIN;
}


// Write data out over one wire
int8_t ker_one_wire_write(uint8_t *command_data, uint8_t command_size,
        uint8_t* write_data, uint8_t write_size)
{
    int i;

    if (start_comm() != PRESENCE) {
        return -EBUSY;
    }

    // Send command stored in write_data
    for (i = 0; i < command_size; i++) {
        write_byte(*(command_data+i));
    }
    
    // Write buffer of data
    for (i = 0; i < write_size; i++) {
        write_byte(*(write_data+i));
    }

    return SOS_OK;
}


// Send command and then read in data over one wire
int8_t ker_one_wire_read(uint8_t *command_data, 
        uint8_t command_size, 
        uint8_t *read_data,
        uint8_t read_size)
{
    int i;

    if (start_comm() != PRESENCE) {
        return -EBUSY;
    }

    // Send command stored in write_data
    for (i = 0; i < command_size; i++) {
        write_byte(*(command_data+i));
    }
    
    // Read data from the bus
    for (i = 0; i < read_size; i++)
    {
        *(read_data + i) = read_byte();
    }

    return SOS_OK;
}

