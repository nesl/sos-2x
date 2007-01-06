#ifndef ADCM1700WINDOWSIZE_H
#define ADCM1700WINDOWSIZE_H

#include "image.h"


extern int8_t adcm1700WindowSizeInit();
extern int8_t adcm1700WindowSizeSetInputSize(wsize_t iwsize);
extern int8_t adcm1700WindowSizeSetOutputSize(wsize_t owsize);
extern int8_t adcm1700WindowSizeSetInputPan(wpos_t iwpan);


#endif

