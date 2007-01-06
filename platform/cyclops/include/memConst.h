////////////////////////////////////////////////////////////////////////////
// 
// CENS 
// Agilent
//
// Copyright (c) 2003 The Regents of the University of California.  All 
// rights reserved.
//
// Copyright (c) 2003 Agilent Corporation 
// rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// - Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// - Neither the name of the University nor Agilent Corporation nor 
//   the names of its contributors may be used to endorse or promote 
//   products derived from this software without specific prior written 
//   permission.
//
// THIS SOFTWARE IS PROVIDED BY THE REGENTS , AGILENT CORPORATION AND 
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
// AGILENT CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
//          
//          
//
////////////////////////////////////////////////////////////////////////////
//
// $Header: /Volumes/Vol1/neslcvs/CVS/sos-1.x/platform/cyclops/include/memConst.h,v 1.2 2006/04/28 23:29:23 kevin Exp $
//
// Authors : 
//           Mohammad Rahimi mhr@cens.ucla.edu 
//
////////////////////////////////////////////////////////////////////////////

#ifndef MEMCONST_H
#define MEMCONST_H


//segments are 256 bytes. Memory pages can be the same size of 
//the segment or twice or 4 time the size of the segment. 
enum
{
  SEGMENT_SIZE=256,
  PAGE_SIZE_256=0,
  PAGE_SIZE_512=1,
  PAGE_SIZE_1024=2
};

enum
{
  MEMORY_FREED,
  MEMORY_IN_USE
};

#define INTERNAL_MEM_END 0xFFF
#define EXTERNAL_MEM_START 0x1000
#define END_OF_MEMORY 0xFFFF
#define MAX_ALLOCATED_MEMORY 100
#define ALLOCATABLE_MEMORY_START EXTERNAL_MEM_START+((sizeof(memAlloc_t) * MAX_ALLOCATED_MEMORY )>8 +1) << 8

// buffer is a pointer-to-pointer so that the original buffer
// can move arround
typedef uint8_t** buffer;
typedef buffer* bufferPtr;


struct memAlloc_s 
{
  buffer  memBuffer;
  uint8_t memPageSize;
  uint8_t memStatus;
}__attribute__((packed));

typedef struct memAlloc_s memAlloc_t;


#endif
