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
 * \author Roy Shea
 *
 * \todo This should only be a template with actual instantiations moved to
 * platform directories...
 */

#ifndef _SYS_ONE_WIRE_H
#define _SYS_ONE_WIRE_H

#define ONE_WIRE_PORT PORTC
#define ONE_WIRE_DIRECTION DDRC
#define ONE_WIRE_READ_PIN PINC
#define ONE_WIRE_PIN_NUMBER 7
#define ONE_WIRE_PIN 0x80

// XXX Beware:
// Obtained from scope specificly for mica2.  The following function gives:
//
// waitTime = 1.09 * x 
//
// Note that optimizations or lack there of could effect this timing...

inline void ds2438_uwait(uint16_t _u_sec) {        
    while (_u_sec > 0) {            
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      asm volatile  ("nop" ::);     
      _u_sec--;                     
    }                               
}


#endif // _SYS_ONE_WIRE_H


