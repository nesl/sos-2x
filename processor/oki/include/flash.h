/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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

#ifndef _FLASH_H
#define _FLASH_H

#include <sos_types.h>
#include <sos_module_types.h>

void flash_init(void);

/**
 * @brief program a page in the flash ROM.
 *
 * @param page The page number to program, 0..479.
 * @param data Pointer to the new page contents.
 * @param len  Number of bytes to program. If this is
 *             less than 256, then the remaining bytes
 *             will be filled with 0xFF.
 */
void FlashWritePage(uint16_t page, uint8_t *data, uint16_t len) __attribute__ ((section (".sos_bls")));

/**
 * @brief Copy data into SPM buffer
 * @param page The page number to read
 * @param start the starting address in the page
 * @param data  data pointer
 * @param len   data length
 * Note that this routine assumes length is even
 */
void FlashReadPage(uint16_t page, uint16_t start, uint8_t *data, uint16_t size) __attribute__ ((section (".sos_bls")));


/**
 * @brief load flash buffer from flash
 * @param page The page number to check
 * @param data data pointer to be checked against
 * @param size number of bytes in data
 *
 */
int8_t FlashCheckPage(uint16_t page, uint8_t *data, uint16_t size) __attribute__ ((section (".sos_bls")));

int CopyFuncToRam(unsigned long *begin,unsigned long *end,unsigned long *copy,int size);

int fmGetFlashSize(void);

unsigned long fmGetFlashStart(void);

//! XXX XXX
#define FlashGetProgmem(addr) (addr)
#endif
