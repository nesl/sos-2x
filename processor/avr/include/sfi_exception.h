/**
 * \file sfi_exception.h
 * \brief SFI Exception Codes for Display
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _SFI_EXCEPTION_H_
#define _SFI_EXCEPTION_H_

#define  KER_RET_CHECK_EXCEPTION      1

#define  KER_ICALL_CHECK_EXCEPTION    2

#define  MEMMAP_STACK_WRITE_EXCEPTION 3
#define  MEMMAP_HEAP_WRITE_EXCEPTION  3 // Note: Same as above
 
#define  KER_VERIFY_FAIL_EXCEPTION    4

#define  MALLOC_EXCEPTION             5

#define  SFI_DOMAINID_EXCEPTION       6

#endif//_SFI_EXCEPTION_H_
