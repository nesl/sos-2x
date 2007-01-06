#ifndef IMG_BACKGROUND_H
#define IMG_BACKGROUND_H

#include <sos_types.h>
#include <image.h>
#include <malloc_extmem.h>
#include "matrix.h"

typedef struct background_update
{
	CYCLOPS_Matrix *  matPtrA; 
	uint8_t value ; 
} background_update_t;

#ifndef _MODULE_

extern int8_t updateBackground(const CYCLOPS_Matrix* newImage, CYCLOPS_Matrix* background, uint8_t coeff);
extern double estimateAvgBackground(const CYCLOPS_Matrix* A, uint8_t skip);
extern int8_t OverThresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh);


#endif

#endif

