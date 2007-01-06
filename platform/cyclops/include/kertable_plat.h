#ifndef _KERTABLE_PLAT_H_
#define _KERTABLE_PLAT_H_

#include <kertable_proc.h>

/*
 * This file is accessed from module_plat.h for constant
 * definitions, so need #ifdef to exclude the kertable when
 * it is compiled for a module.
 */

#ifndef _MODULE_
#include <imgBackground.h>
#include <basicStat.h>
#include <matrix.h>
#include <malloc_extmem.h>
#include <adcm1700Control.h>

#define PLAT_KER_TABLE					\
  /*  1 */ NULL,					\
    /*  2 */ NULL,					\
    /*  3 */ NULL,					\
    /*  4 */ (void*)ker_releaseImagerClient,		\
    /*  5 */ (void*)ker_led,				\
    /*  6 */ (void*)ker_snapImage,			\
    /*  7 */ (void*)ker_setCaptureParameters,		\
    /*  8 */ (void*)ker_registerImagerClient,		\
    /*  9 */ (void*)ext_mem_get_handle,			\
    /* 10 */ (void*)ext_mem_free_handle,		\
    /* 11 */ (void*)ker_get_handle_ptr,			\
    /* 12 */ (void*)ext_mem_init,	\
    /* 13 */ (void*)convertImageToMatrix,		\
   /* 14 */ (void*)updateBackground,		\
    /* 15 */ (void*)abssub,		\
    /* 16 */ (void*)estimateAvgBackground,		\
    /* 17 */ (void*)maxLocate,		\
   /* 18 */ (void*)OverThresh,			
    
#endif    
    
//#define PLAT_KERTABLE_LEN 12
#define PLAT_KERTABLE_LEN 18
#define PLAT_KERTABLE_END (PROC_KERTABLE_END+PLAT_KERTABLE_LEN)

#endif
