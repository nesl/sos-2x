/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA,
 * 94704.  Attention:  Intel License Inquiry.
 */

/**
 * @brief linear estimator for RATS
 * @author Ilias Tsigkogiannis {ilias@ee.ucla.edu}
 */
 
#include <inttypes.h>
#include <systime.h>
#include "rats.h"
#include "math.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DEFAULT_START_VALUE 1

float normalize(float* pTSParentArray, float* pTSMyArray, uint8_t max_window, uint8_t size, uint8_t slot);

//The following functions are used to convert from clock ticks to milliseconds
//and the opposite. The used clock frequency is supposed to be 115.2KHz.
static inline float ticks_to_msec_float(int32_t ticks) 
{
	return ticks/115.2;
}

static inline float msec_to_ticks_float(int32_t msec) 
{
	return msec*115.2;
}				
	
/**
* Calculate linear regression parameter, and 
*
**/
void getRegression(uint32_t* pTSParentArray1, uint32_t* pTSMyArray1, uint8_t max_window, uint8_t size, 
				uint8_t slot, float* alpha, float* beta) 
{
	uint8_t pos = 0, cnt = 0;
	float A=0.0, B=0.0, C=0.0, D=0.0;   // sum(Tp), sum(Tm), sum(Tm^2), sum(TpTm)
    float pTSParentArray[BUFFER_SIZE], pTSMyArray[BUFFER_SIZE];
	float temp = 0.0, scaling = 0.0;
	
	//Convert timestamping values from timer ticks to microseconds
	for(cnt = 0; cnt < BUFFER_SIZE; cnt++)
	{
		pTSParentArray[cnt] = ticks_to_msec_float(pTSParentArray1[cnt]);
		pTSMyArray[cnt] = ticks_to_msec_float(pTSMyArray1[cnt]);
	}	
			
	scaling = normalize(pTSParentArray, pTSMyArray, max_window, size, slot);

	pos = slot;
	for (cnt = 0; cnt < size; cnt++) 
	{
		A += pTSParentArray[pos];
		B += pTSMyArray[pos];
		C += pTSMyArray[pos] * pTSMyArray[pos];
		D += pTSMyArray[pos] * pTSParentArray[pos];

		pos = (pos+1)%max_window;
	}
	
	// get a and b of y = a + bx
	temp = size*C-B*B;
	
	*alpha = ((A*C-B*D) / temp);
	*beta = ((size*D-B*A) / temp);

	*alpha = *alpha + (1.0 - (*beta))*scaling;
}
   
float getError(uint32_t* pTSParentArray1, uint32_t* pTSMyArray1, uint8_t max_window, uint8_t size, 
				uint8_t slot, float* alpha1, float* beta1, uint16_t period /* sec */, uint8_t should_invert)
{
	uint8_t pos = slot, cnt = 0;
	float errrange;
	float diff = 0.0, sumofdiff = 0.0, sumofdiff2 = 0.0, sumofx = 0.0, sumofdiffx2 = 0.0;
	float avex = 0.0, med = 0.0, var = 0.0, sig = 0.0;
	float PrevAlpha = *alpha1, PrevBeta = *beta1;
    float pTSParentArray[BUFFER_SIZE], pTSMyArray[BUFFER_SIZE];	
	float esterror = 0.0, realerror = 0.0;
	float current_parent = 0.0, current_mine = 0.0, PrevMine = 0.0, PrevParent = 0.0;
	float next_m = 0.0;
	float alpha = 0.0, beta = 0.0;
	uint32_t *pTempArray1, *pTempArray2;
	
	if(should_invert == TRUE)
	{
		pTempArray1 = 	pTSMyArray1;
		pTempArray2 = 	pTSParentArray1;
	}
	else
	{
		pTempArray1 = 	pTSParentArray1;
		pTempArray2 = 	pTSMyArray1;
	}
		
	
	for(cnt = 0; cnt < BUFFER_SIZE; cnt++)
	{
		pTSParentArray[cnt] = ticks_to_msec_float(pTempArray1[cnt]);
		pTSMyArray[cnt] = ticks_to_msec_float(pTempArray2[cnt]);
	}		
	
	// calculate real error
	if ((PrevAlpha == 0.0) && (PrevBeta == 0.0)) 
	{ 
		// nothing
	}
	else
	{
		if(should_invert == TRUE)
		{
			alpha = - (*alpha1)/(*beta1);
			beta = 1/(*beta1);
		}
		else
		{
			alpha = *alpha1;
			beta = *beta1;	
		}
		
		
		realerror = (PrevAlpha + PrevBeta * pTSMyArray[slot+size-1]) - pTSParentArray[slot+size-1];
      
		if (realerror < 0) 
			realerror = -1.0 * realerror;
	}

	normalize(pTSParentArray, pTSMyArray, max_window, size, slot);
	
	pos = slot;
	for (cnt = 0; cnt < size; cnt++) 
	{
		PrevParent = current_parent;
      	current_parent = pTSParentArray[pos];
		PrevMine = current_mine;
		current_mine = pTSMyArray[pos];
		
		pos = (pos+1)%max_window;
	}	
	
	// next estimated ts    
	next_m = current_mine + ( current_mine - PrevMine);	
	
	pos = slot;
	for (cnt = 0; cnt < size; cnt++) 
	{
		// calculate diff between real p and expected p 
		diff = pTSParentArray[pos] - (alpha + ( pTSMyArray[pos] * (beta) ) );

		sumofdiff += diff;
		sumofdiff2 += (diff * diff);
		sumofx += pTSMyArray[pos];
		pos = (pos+1)%max_window;
	}
    
	avex = sumofx / size;
	
	pos = slot;
	for (cnt = 0; cnt < size; cnt++) 
	{
		sumofdiffx2 += (pTSMyArray[pos] - avex)*(pTSMyArray[pos] - avex);
		pos = (pos+1)%max_window;
	}

	med = sumofdiff / size;
	var = (sumofdiff2 / size) - (med * med);
	
	if(var < 0)
		var = (-1)*var;
	
	sig = sqrt(var);
    
	esterror = 2.31 * sig * sqrt( (1.0 + 1.0/size) + ((next_m - avex)*(next_m - avex))/sumofdiffx2);
	
	if (period <= 512)
		esterror *= 2.0;   // period = 1,2,4,8 min
	else if (period <= 2048)
		esterror *= 3.0;   // period = 16 min
	else 
		esterror *= 4.0;   // period = 32  
      
	errrange = esterror;

//	if (realerror > esterror)
//		*errrange = realerror;

	return errrange;
}

float normalize(float* pTSParentArray, float* pTSMyArray, uint8_t max_window, uint8_t size, uint8_t slot)
{
	// linear regression
	uint8_t pos = slot, cnt = 0;
	float current_parent = 0.0, current_mine = 0.0, PrevMine = 0.0, PrevParent = 0.0;
	float scaling = 0.0;
	
	pos = slot;
	for (cnt = 0; cnt < size; cnt++) 
	{
		if (pTSParentArray[pos] < current_parent)
		{
			pTSParentArray[pos] += FLOAT_MAX_GTIME;	
		}
		PrevParent = current_parent;
      	current_parent = pTSParentArray[pos];
		
		if (pTSMyArray[pos] < current_mine)
		{
			pTSMyArray[pos] += FLOAT_MAX_GTIME;
		}
		PrevMine = current_mine;
		current_mine = pTSMyArray[pos];
		
		pos = (pos+1)%max_window;
	}	

	//Normalize all values so that smallest one is equal to DEFAULT_START_VALUE
	if((pTSParentArray[slot] != 0) && (pTSMyArray[slot] != 0))
	{
		if (pTSParentArray[slot] < pTSMyArray[slot])
			scaling = pTSParentArray[slot] - DEFAULT_START_VALUE;
		else
			scaling = pTSMyArray[slot] - DEFAULT_START_VALUE;
	}
	else
		scaling = 0;
		
	pos = slot;	

	for(cnt = 0; cnt < BUFFER_SIZE; cnt++)
	{
		pTSParentArray[cnt] -= scaling;
		pTSMyArray[cnt] -= scaling;
	}
	return scaling;
}
