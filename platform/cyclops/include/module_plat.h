/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/** 
 * @brief Header file for mica2 specific modules
 */
#ifndef _MODULE_PLAT_H
#define _MODULE_PLAT_H

// This file is automatically included from module.h
// if _MODULE_ is defined.

#include <image.h> // Cyclops Platform Image Data Type Definitions
#include <kertable_plat.h> // Required for PROC_KERTABLE_END
#include <matrix.h> // Required for CYCLOPS_Matrix etc. data types
#include <hardware_types.h>

/**
 * @brief led functions
 * @param action can be following
 *    LED_RED_ON
 *    LED_GREEN_ON
 *    LED_YELLOW_ON
 *    LED_RED_OFF
 *    LED_GREEN_OFF
 *    LED_YELLOW_OFF
 *    LED_RED_TOGGLE
 *    LED_GREEN_TOGGLE
 *    LED_YELLOW_TOGGLE
 *    LED_AMBER_ON
 *    LED_AMBER_OFF
 *    LED_AMBER_TOGGLE
 */
typedef int8_t (*ledfunc_t)(uint8_t action);
static inline int8_t ker_led(uint8_t action){
	ledfunc_t func = (ledfunc_t)get_kertable_entry(PROC_KERTABLE_END+5);
	return func(action);
}


typedef int8_t (*snapImage_t)(CYCLOPS_ImagePtr ptrImg);
static inline int8_t ker_snapImage(CYCLOPS_ImagePtr ptrImg){
	snapImage_t func = (snapImage_t)get_kertable_entry(PROC_KERTABLE_END+6);
	return func(ptrImg);
}

typedef int8_t (*setCaptureParameters_t)(CYCLOPS_CapturePtr pCap);
static inline int8_t ker_setCaptureParameters(CYCLOPS_CapturePtr pCap){
	setCaptureParameters_t func = (setCaptureParameters_t)get_kertable_entry(PROC_KERTABLE_END+7);
	return func(pCap);
}

typedef int8_t (*registerImagerClient_t)(sos_pid_t pid);
static inline int8_t ker_registerImagerClient(sos_pid_t pid){
	registerImagerClient_t func = (registerImagerClient_t)get_kertable_entry(PROC_KERTABLE_END+8);
	return func(pid);
}

typedef int8_t (*releaseImagerClient_t)(sos_pid_t pid);
static inline int8_t ker_releaseImagerClient(sos_pid_t pid){
	registerImagerClient_t func = (registerImagerClient_t)get_kertable_entry(PROC_KERTABLE_END+4);
	return func(pid);
}

typedef int16_t (*ext_mem_get_handle_t)(uint16_t size, sos_pid_t id, bool bCallFromModule);
static inline int16_t ker_get_handle(uint16_t size, sos_pid_t id){
	ext_mem_get_handle_t func = (ext_mem_get_handle_t)get_kertable_entry(PROC_KERTABLE_END + 9);
	return func(size, id, true);
}

typedef int8_t (*ext_mem_free_handle_t)(int16_t handle, bool bCallFromModule);
static inline int8_t ker_free_handle(int16_t handle){
	ext_mem_free_handle_t func = (ext_mem_free_handle_t)get_kertable_entry(PROC_KERTABLE_END + 10);
	return func(handle, true);
}

typedef void* (*get_handle_ptr_t)(int16_t handle);
static inline void* ker_get_handle_ptr(int16_t handle){
	get_handle_ptr_t func = (get_handle_ptr_t)get_kertable_entry(PROC_KERTABLE_END + 11);
	return func(handle);
}

///////////////////////////////////////////

typedef int8_t (*convertImageToMatrix_t)(CYCLOPS_Matrix* M, const CYCLOPS_Image* Im);
static inline int8_t convertImageToMatrix(CYCLOPS_Matrix* M, const CYCLOPS_Image* Im){
	convertImageToMatrix_t func = (convertImageToMatrix_t)get_kertable_entry(PROC_KERTABLE_END + 13);
	return func(M, Im);
}
//extern int8_t convertImageToMatrix(CYCLOPS_Matrix* M, const CYCLOPS_Image* Im);

typedef int8_t (*updateBackground_t)(const CYCLOPS_Matrix* newImage, CYCLOPS_Matrix* background, uint8_t coeff);
static inline int8_t updateBackground(const CYCLOPS_Matrix* newImage, CYCLOPS_Matrix* background, uint8_t coeff){
	updateBackground_t func = (updateBackground_t)get_kertable_entry(PROC_KERTABLE_END + 14);
	return func(newImage, background, coeff);
}
//extern int8_t updateBackground(const CYCLOPS_Matrix* newImage, CYCLOPS_Matrix* background, uint8_t coeff);


typedef int8_t (*abssub_t)(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C);
static inline int8_t abssub(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C){
	abssub_t func = (abssub_t)get_kertable_entry(PROC_KERTABLE_END + 15);
	return func(A, B, C);
}
//extern int8_t abssub(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C);


typedef double (*estimateAvgBackground_t)(const CYCLOPS_Matrix* A, uint8_t skip);
static inline double estimateAvgBackground(const CYCLOPS_Matrix* A, uint8_t skip){
	estimateAvgBackground_t func = (estimateAvgBackground_t)get_kertable_entry(PROC_KERTABLE_END + 16);
	return func(A, skip);
}
//extern double estimateAvgBackground(const CYCLOPS_Matrix* A, uint8_t skip);

typedef int8_t (*maxLocate_t)(CYCLOPS_Matrix* A, uint8_t* row, uint8_t* col);
static inline int8_t maxLocate(CYCLOPS_Matrix* A, uint8_t* row, uint8_t* col){
	maxLocate_t func = (maxLocate_t)get_kertable_entry(PROC_KERTABLE_END + 17);
	return func(A, row, col);
}
//extern int8_t maxLocate(CYCLOPS_Matrix* A, uint8_t* row, uint8_t* col);

typedef int8_t (*OverThresh_t)(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh);
static inline int8_t OverThresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh){
	OverThresh_t func = (OverThresh_t)get_kertable_entry(PROC_KERTABLE_END + 18);
	return func(A, row, col, range, thresh);
}
//extern int8_t OverThresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh);



typedef int8_t (*ext_mem_init_t)();
static inline int8_t ext_mem_init(){
	ext_mem_init_t func = (ext_mem_init_t)get_kertable_entry(PROC_KERTABLE_END + 12);
	return func();
}

//int8_t ext_mem_init();
#endif /* _MODULE_PLAT_H */

