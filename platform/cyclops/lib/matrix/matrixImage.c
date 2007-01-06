/*
 *Authors: 
 * Obi Iroezi: obimdina@seas.ucla.edu
 * Juan Carlos Garcia: yahualic@ucla.edu
 * Mohammad Rahimi: mhr@cens.ucla.edu
 *
 *This file contains the implementation for the commands
 *to convert images to matrices.
 */

#include <matrix.h>
#include <image.h>
//#include <malloc_extmem.h>

int8_t
convertImageToMatrix (CYCLOPS_Matrix * M, const CYCLOPS_Image * Im)
{
  if (Im->type == CYCLOPS_IMAGE_TYPE_Y)
    {
      if ((Im->size.x * Im->size.y) <= (M->rows * M->cols))
	{			//test that the matrix is big enough for the image
	  M->depth = CYCLOPS_1BYTE;
	  M->rows = Im->size.x;
	  M->cols = Im->size.y;
	  //Kevin - tricky! watch out!
	  M->data.hdl8 = Im->imageDataHandle;
	  return SOS_OK;
	}
      else
	return -EINVAL;
    }
  else
    return -EINVAL;
}

int8_t
convertRGBToMatrix (CYCLOPS_Matrix * M, const CYCLOPS_Image * Im,
		    uint8_t color)
{
  uint8_t *imgPtr;		//points to the image
  uint8_t *matPtr;		//points to the matrix
  uint16_t size = Im->size.x * Im->size.y;
  uint16_t i;
  if (Im->type == CYCLOPS_IMAGE_TYPE_RGB)

    {
      if ((Im->size.x * Im->size.y) <= (M->rows * M->cols))

	{
	  M->depth = CYCLOPS_1BYTE;
	  M->rows = Im->size.x;
	  M->cols = Im->size.y;
	  //Kevin - tricky! watch out!

	  imgPtr = ker_get_handle_ptr (Im->imageDataHandle);
	  matPtr = ker_get_handle_ptr (M->data.hdl8);
	  if ((imgPtr == NULL) || (matPtr == NULL))
	    {
	      return -EINVAL;
	    }

	  imgPtr += color;

	  //matPtr = M->data.ptr8;
	  for (i = 0; i < size; i++)
	    {
	      *matPtr = *imgPtr;
	      matPtr++;
	      imgPtr += 3;
	    }
	  return SOS_OK;
	}
    }
  return -EINVAL;
}
