/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
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
 * @brief    Allocte and free dynamic memory 
 * @author   Roy Shea (roy@cs.ucla.edu) 
 */
#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <sos_types.h>
#include <pid.h>
#include <malloc_conf.h>
#include <sos_module_types.h>
#ifdef FAULT_TOLERANT_SOS
#include <malloc_domains.h>
#endif

/**
 * @brief Init function for memory manager
 */
extern void mem_init(void);

/**
 * @brief Starting memory module interface
 */
extern void mem_start(void);

/**
 * @brief Allocate a chunk of blocks from the heap
 */
extern void *sos_blk_mem_alloc(uint16_t size, sos_pid_t id, bool bCallFromModule);

/**
 * @brief Free a block back into the heap
 */
extern void sos_blk_mem_free(void* ptr, bool bCallFromModule);

/**
 * @brief Re-allocate a block of memory from the heap
 */
extern void *sos_blk_mem_realloc(void* pntr, uint16_t newSize, bool bCallFromModule);

/**
 * @brief Change memory ownership of a segment of memory
 */
extern int8_t sos_blk_mem_change_own(void* ptr, sos_pid_t id, bool bCallFromModule);

/**
 * @brief Allocate a block of memory for long term usage
 */
extern void *sos_blk_mem_longterm_alloc(uint16_t size, sos_pid_t id, bool bCallFromModule);


/**
 * @brief Allocate dynamic memory
 * @param size Number of bytes to allocate
 * @param id Node responsible for the memory
 * @return Returns a pointer to the allocated memory.
 * Will return a NULL pointer if the call to sys_malloc fails.
 */
static inline void *ker_malloc(uint16_t size, sos_pid_t id)
{
  return sos_blk_mem_alloc(size, id, false);
}

/**
 * @brief Reallocate dynamic memory
 * @param pntr Pointer to the currently held block of memory
 * @param newSize Number of bytes to be allocated
 * @return Returns the pointer to the reallocated memory.
 * Returns a NULL if unable to reallocate but the original pointer is still valid.
 */
static inline void* ker_realloc(void* pntr, uint16_t newSize)
{
  return sos_blk_mem_realloc(pntr, newSize, false);
}

/**
 * @brief Free memory pointed to by ptr
 * @param ptr Pointer to the memory that should be released
 * @return void
 */
static inline void ker_free(void* ptr)
{
  sos_blk_mem_free(ptr, false);
  return;
}

/**
 * @brief Change the ownership of memory
 * @param ptr Pointer to the memory whose ownership is being transferred
 * @param id New owner of the memeory
 * @return SOS_OK or error code upon fail
 * Add check to prevent a change of ownership to the 'null' user.
 */
static inline int8_t ker_change_own(void* ptr, sos_pid_t id)
{
  return sos_blk_mem_change_own(ptr, id, false);
}


extern sos_pid_t mem_check_memory();

/**
 * @brief Free up all memory held by id 
 * @param id Process that is having its memory returned
 */
extern int8_t mem_remove_all(sos_pid_t id);

/**
 * @brief malloc for long term usage
 * @warning this is used to allocate the memory for long time usage
 */
static inline void* malloc_longterm(uint16_t size, sos_pid_t id)
{
  return sos_blk_mem_longterm_alloc(size, id, true);
}

/**
 * Mark the memory is valid (Used by the kernel only)
 */
extern int8_t ker_gc_mark( sos_pid_t pid, void *pntr );

/**
 * GC a module (Used by the kernel only)
 *
 * @note to GC a loadable module, one should always use malloc_gc_module
 */
extern void malloc_gc(sos_pid_t pid);

/**
 * Entry point for GC the entire kernel
 * 
 * The interface is intended to be called by malloc.c
 */
extern void malloc_gc_kernel( void );

/**
 * GC a loadable module 
 * 
 * The interface is intended to be called by malloc.c
 */
extern uint8_t malloc_gc_module( sos_pid_t pid );
#ifndef FAULT_TOLERANT_SOS
#define ker_valid_access NULL
#endif //FAULT_TOLERANT_SOS

#endif

