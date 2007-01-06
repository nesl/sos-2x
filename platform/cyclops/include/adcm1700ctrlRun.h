#ifndef ADCM1700CTRLRUN_H
#define ADCM1700CTRLRUN_H

#include "image.h"

extern int8_t adcm1700CtrlRunInit();
extern int8_t adcm1700CtrlRunCamera(uint8_t run_stop);
extern int8_t adcm1700CtrlRunSensor(uint8_t run_stop);



#endif

