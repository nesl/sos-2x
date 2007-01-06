#ifndef BASIC_STAT_H
#define BASIC_STAT_H

//#include <sos_types.h>
//#include <image.h>
//#include <malloc_extmem.h>

#ifndef _MODULE_

#include <matrix.h>


extern int16_t min(CYCLOPS_Matrix* A);
extern int16_t max(CYCLOPS_Matrix* A);
extern int8_t maxLocate(CYCLOPS_Matrix* A, uint8_t* row, uint8_t* col);
extern double avg(CYCLOPS_Matrix* A);

#endif

#endif

