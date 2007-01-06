/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License 
 *
 *  Copyright (c) 2002 Intel Corporation 
 *  All rights reserved. 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 * 
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *      Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTEL OR ITS
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/**
 * @author Phil Buonadonna
 * @author Simon Han
 *   Based on CC1000Const.h
 *   Port to SOS
 */


#ifndef _SOS_CC1K_LPL_H
#define _SOS_CC1K_LPL_H

// duty cycle         max packets        effective throughput
// -----------------  -----------------  -----------------
// 100% duty cycle    42.93 packets/sec  12.364kbps
// 35.5% duty cycle   19.69 packets/sec   5.671kbps
// 11.5% duty cycle    8.64 packets/sec   2.488kbps
// 7.53% duty cycle    6.03 packets/sec   1.737kbps
// 5.61% duty cycle    4.64 packets/sec   1.336kbps
// 2.22% duty cycle    1.94 packets/sec   0.559kbps
// 1.00% duty cycle    0.89 packets/sec   0.258kbps
static const prog_uchar CC1K_LPL_PreambleLength[CC1K_LPL_STATES*2] = {
    0, 8,      //28
    0, 94,      //94
    0, 250,     //250
    0x01, 0x73, //371,
    0x01, 0xEA, //490,
    //0x04, 0xBC, //1212
    0x5, 0x55, // 1365
    0x0A, 0x5E  //2654
};

static const prog_uchar CC1K_LPL_SleepTime[CC1K_LPL_STATES*2] = {
    0, 0,       //0
    0, 20,      //20
    0, 85,      //85
    0, 135,     //135
    0, 185,     //185
    //0x01, 0xE5, //485
    0x2, 0x2C, //556
    0x04, 0x3D  //1085
};

static const prog_uchar CC1K_LPL_SleepPreamble[CC1K_LPL_STATES + 1] = {
    0, 
    8,
    8,
    8, 
    8,
    12,
    8
};



#endif // _SOS_CC1K_LPL_H

