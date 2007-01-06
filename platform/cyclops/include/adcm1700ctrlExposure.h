#ifndef ADCM1700CTRLEXPOSURE_H
#define ADCM1700CTRLEXPOSURE_H

#include "image.h"

extern int8_t adcm1700CtrlExposureInit();
extern int8_t adcm1700CtrlSetExposureTime(float exposureTime);
extern int8_t adcm1700CtrlSetAnalogGain(color8_t analogGain);
extern int8_t adcm1700CtrlSetDigitalGain(color16_t digitalGain);



#endif

