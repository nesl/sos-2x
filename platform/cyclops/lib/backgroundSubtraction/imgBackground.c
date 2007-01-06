/*
 *Authors: 
 * Obi Iroezi: obimdina@seas.ucla.edu
 * Juan Carlos Garcia: yahualic@ucla.edu
 * Mohammad Rahimi: mhr@cens.ucla.edu
 *
 *This file contains the implementation for the commands
 *to subtract the background from an image.
 */

#include "matrix.h"
#include <imgBackground.h>

int8_t updateBackground(const CYCLOPS_Matrix* newImage, CYCLOPS_Matrix* background, uint8_t coeff)
{
	//check that input matrix's depth is 1 byte
	if( (newImage->depth != CYCLOPS_1BYTE) || (background->depth != CYCLOPS_1BYTE) )
		return -EINVAL;
	//check that the coefficient is 1,2,3 or 4.  Otherwise -EINVAL.
	if( (coeff != 1) && (coeff != 2) && (coeff != 3) && (coeff != 4) )
		return -EINVAL;
	else
	{
		//check that input matrix's sizes are equal
		if( (newImage->rows != background->rows) ||
			(newImage->cols != background->cols) )
			return -EINVAL;
		else
		{				
			uint8_t *newImg_ptr8;
			uint8_t *backGrnd_ptr8;
			newImg_ptr8 = ker_get_handle_ptr (newImage->data.hdl8);
			backGrnd_ptr8 = ker_get_handle_ptr (background->data.hdl8);
			  if ((newImg_ptr8 == NULL) || (backGrnd_ptr8 == NULL))
			{
				return -EINVAL;
			}											
			
			uint16_t max = newImage->rows * newImage->cols;
			uint16_t i = 0;  //counter
			
			for( i = 0; i < max; i++)
				backGrnd_ptr8[i] = (newImg_ptr8[i] >> coeff) + ( (1 << coeff) - 1)*(backGrnd_ptr8[i] >> coeff);
			return SOS_OK;
		}
	}
}

double estimateAvgBackground(const CYCLOPS_Matrix* A, uint8_t skip)
{
	//check the depth
	if(A->depth != CYCLOPS_1BYTE)
		return 0;  //-EINVAL
	else
	{
		uint8_t i,j;  //row and col counter
		uint16_t samples = 0;  //how many samples we collected
		uint32_t total = 0;
		
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}		
		
		for( i=0; i < A->rows; i += skip)
		{
			for(j=0; j < A->cols; j += skip)
			{
				total += A_ptr8[ (i * A->cols) + j];
				samples++;
			}
		}
		return ( (double)total )/ samples;
	}
}

int8_t OverThresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh)
{
	//check the depth, rows and cols
	if( (A->depth != CYCLOPS_1BYTE) || (A->rows < row) || (A->cols < col) )
	{
		return 0xff;  //-EINVAL.  We used this instead of zero since zero might mean
		//that no pixel was above the threshold.
	}
	else
	{
		uint8_t startCol = col - range;
		uint8_t endCol = col + range;
		uint8_t startRow = row - range;
		uint8_t endRow = row + range;
		uint8_t i,j, counter=0; //counters
						
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}	
		if( row + range > A->rows )  //Boundary checks
			endRow = A->rows;
		if( row < range)
			startRow = 0;
		if( col + range > A->cols)
			endCol = A->cols;
		if( col < range)
			startCol = 0;
		for( i = startRow; i <=endRow; i++ )
			for( j = startCol; j<=endCol; j++)
				if( A_ptr8[ i * A->cols + j] > thresh)
					counter++;
		return counter;
	}
}
