/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */


#ifndef _MTS300SB_H_
#define _MTS300SB_H_

#ifndef MTS300SB
#define MTS300SB
#endif

/**
 * get shared include for all mts3XX sensor boards
 */
#include <mtx3XXsb.h>

#ifndef MTS300SB
#define MTS300SB
#endif

// sensor port id's
#define MTS300_PHOTO_SID   MTS3XX_PHOTO_SID
#define MTS300_TEMP_SID    MTS3XX_TEMP_SID
#define MTS300_MIC_SID     MTS3XX_MIC_SID

// sensor types
#define MTS300_PHOTO_TYPE  MTS3XX_PHOTO_TYPE
#define MTS300_TEMP_TYPE   MTS3XX_TEMP_TYPE
#define MTS300_MIC_TYPE    MTS3XX_MIC_TYPE

// ADC port mapping for mts300
#define MTS300_PHOTO_ADC   MTS3XX_PHOTO_ADC
#define MTS300_TEMP_ADC    MTS3XX_TEMP_ADC

#define MTS300_MIC_ADC     MTS3XX_MIC_ADC

#endif // _MTS300SB_H_

