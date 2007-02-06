/*									tab:4
 *
 *
 * "Copyright (c) 2000-2004 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without written
 * agreement is hereby granted, provided that the above copyright
 * notice, the following two paragraphs and the author appear in all
 * copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 */
/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License 
 *
 *  Copyright (c) 2004 Intel Corporation 
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
 * 
 */
/* Authors:   Rahul Balani
 * History:   April 2005	Ported to sos
 *            November 2005	Modifications for new design
 */

/**
 * @author Rahul Balani
 */


#ifndef _DVM_H_
#define _DVM_H_

#include <stddef.h>
#include <sos_list.h>
#include <string.h>
#include <codemem.h>
#include "DvmConstants.h"

typedef uint8_t  DvmOpcode;
typedef uint16_t DvmCapsuleLength;
typedef uint32_t DvmCapsuleVersion;

typedef struct {
  list_t queue;
}  __attribute__((packed)) DvmQueue; 

typedef struct {
  int16_t var;
}  __attribute__((packed)) DvmValueVariable;

typedef struct {
  uint8_t size;
  uint8_t entries[DVM_BUF_LEN];
}  __attribute__((packed)) DvmDataBuffer;

typedef struct {
  DvmDataBuffer* var;
} DvmBufferVariable;

typedef struct {
  uint8_t type;
  union {
    DvmValueVariable value;
    DvmBufferVariable buffer;
  };
} __attribute__((packed)) DvmStackVariable;

/*
typedef struct {
	DvmOpcode data[DVM_CAPSULE_SIZE];
} DvmEventType;
*/

typedef struct {
  uint8_t sp;
  DvmStackVariable stack[DVM_OPDEPTH];
} __attribute__((packed)) DvmOperandStack;
   	 
/* A DvmCapsule is the unit of propagation. */
/*
typedef struct {
  //DvmCapsuleOption options;
  DvmCapsuleLength dataSize;
  int8_t data[DVM_CAPSULE_SIZE];
} __attribute__((packed)) DvmCapsule;
*/

typedef struct {
  uint8_t moduleID;								// Event information
  uint8_t type;
  DvmCapsuleLength dataSize;					// Size of script in bytes
  uint8_t libraryMask;							// Indicates required libraries
  uint16_t pc;									// Current pc value
  uint8_t state;								// Context state
  DvmCapsuleID which;							// Context ID
  uint8_t heldSet[(DVM_LOCK_COUNT + 7) / 8];    // Held locks
  uint8_t releaseSet[(DVM_LOCK_COUNT + 7) / 8]; // Pending lock releases
  uint8_t acquireSet[(DVM_LOCK_COUNT + 7) / 8];	// Locks to acquire
  list_link_t link;								// Link entry for wait queues
  DvmQueue* queue;								// Current wait queue
  uint16_t num_executed;
  uint8_t *init_data;							// Initial data attached with an event message
  uint8_t init_size;
} __attribute__((packed)) DvmContext;

typedef struct {
  DvmContext context;
  DvmOperandStack stack;
  DvmStackVariable vars[DVM_NUM_LOCAL_VARS];
  codemem_t cm;                                 // the handle to codemem
} DvmState;
  
typedef struct {
  DvmContext* holder;
}  __attribute__((packed)) DvmLock;

typedef struct DvmErrorMsg {
  uint8_t context;
  uint8_t reason;
  uint8_t capsule;
  uint8_t instruction;
  uint16_t me;
}  __attribute__((packed)) DvmErrorMsg;

typedef struct DvmTrickleTimer {
  uint16_t elapsed;      // Current time (in ticks)
  uint16_t threshold;    // Time to consider transmitting (in ticks) (t)
  uint16_t interval;     // Size of current interval (in ticks)      (tau)
  uint16_t numHeard;     // Number of messages heard                 (c)
} __attribute__((packed)) DvmTrickleTimer;

typedef struct {
	sos_pid_t destModID;    // lddata requirement: destination module ID
	uint8_t capsuleID;      // lddata requirement: data ID
	sos_pid_t moduleID;
	uint8_t eventType;
	DvmCapsuleLength length;
	uint8_t libraryMask;
	uint8_t data[DVM_MAX_SCRIPT_LENGTH];
} __attribute__((packed)) DvmScript;

typedef struct {
  DvmCapsuleVersion version;
  uint8_t capsuleNum;
  uint8_t piece;
  uint8_t chunk[MVIRUS_CHUNK_SIZE];
}  __attribute__((packed)) DvmCapsuleChunkMsg;

typedef struct {
  DvmCapsuleVersion versions[DVM_CAPSULE_NUM];
}  __attribute__((packed)) DvmVersionMsg;

typedef struct {
  DvmCapsuleVersion version;
  uint8_t capsuleNum;
  uint8_t bitmask[MVIRUS_BITMASK_SIZE];
}  __attribute__((packed)) DvmCapsuleStatusMsg;

#endif
