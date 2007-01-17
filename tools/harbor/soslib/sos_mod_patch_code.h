/**
 * \file sos_mod_patch_code.h
 * \brief Codes for calling the various checking routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _SOS_MOD_PATCH_CODE_H_
#define _SOS_MOD_PATCH_CODE_H_

enum
  {
    KER_RET_CHECK_CODE = 0,
    KER_MEMMAP_PERMS_CHECK_X_CODE = 1,
    KER_MEMMAP_PERMS_CHECK_Y_CODE = 2,
    KER_MEMMAP_PERMS_CHECK_Z_CODE = 3,
    KER_ICALL_CHECK_CODE = 4,
    KER_INTCALL_CODE = 5,
  };

#endif//_SOS_MOD_PATCH_CODE_H_
