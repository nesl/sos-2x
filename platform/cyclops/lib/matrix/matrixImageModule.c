/*
 *Authors: 
 * Obi Iroezi: obimdina@seas.ucla.edu
 * Juan Carlos Garcia: yahualic@ucla.edu
 * Mohammad Rahimi: mhr@cens.ucla.edu
 *
 *This file contains the implementation for the commands
 *to convert images to matrices.
 */

#include <module.h>
#include <matrix.h>
#include <hardware.h>
#include <image.h>
#include <imgBackground.h>



//-----------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------
typedef struct matrix_image_state
{
  background_update_t bckUpdate;
} matrix_image_state_t;

//-----------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------
static int8_t matrix_image_msg_handler (void *start, Message * e);

static int8_t convertImageToMatrix (CYCLOPS_Matrix * M,
				    const CYCLOPS_Image * Im);
static int8_t convertRGBToMatrix (CYCLOPS_Matrix * M,
				  const CYCLOPS_Image * Im, uint8_t color);

//-----------------------------------------------------
// MODULE HEADER
//-----------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id = MATRIX_IMAGE_PID,
  .state_size = sizeof (matrix_image_state_t),
  .num_timers = 1,
  .num_sub_func = 0,
  .num_prov_func = 0,
  .platform_type = HW_TYPE,
  .processor_type = MCU_TYPE,
  .code_id = ehtons (MATRIX_IMAGE_PID),
  .module_handler = matrix_image_msg_handler,
};

//Kevin -  Dont forget to initialize with the app!!!!


int8_t
matrixImage_init ()
{
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

//-------------------------------------------------------
// MESSAGE HANDLER
//-------------------------------------------------------
static int8_t
matrix_image_msg_handler (void *state, Message * msg)
{
  matrix_image_state_t *s = (matrix_image_state_t *) state;

  switch (msg->type)
    {
    case MSG_INIT:
      {
	break;
      }

    case MSG_FINAL:
      {
	break;
      }

      // int8_t convertImageToMatrix (CYCLOPS_Matrix * M, const CYCLOPS_Image * Im) 
    case CONVERT_IMAGE_TO_MATRIX:
      {
	//  Kevin - since framgrabber no longer has access to M, the state should be kept here  
	/*
	   if(first)  //first image collected, backMat should equal image
	   {
	   memcpy(backMat.data.ptr8, M.data.ptr8, M.rows * M.cols);
	   memcpy(objMat.data.ptr8, M.data.ptr8, M.rows * M.cols);
	   first = FALSE;
	   return call Timer.start(TIMER_ONE_SHOT, TIMER_VAL);
	   } 
	 */

	CYCLOPS_Matrix *M;
	CYCLOPS_Image *Im;
	M =
	  (CYCLOPS_Matrix *) ker_malloc (sizeof (CYCLOPS_Matrix),
					 MATRIX_IMAGE_PID);
	if (NULL == M)
	  {
	    return -ENOMEM;
	  }
	Im = (CYCLOPS_Image *) (msg->data);
	if (convertImageToMatrix (M, Im) != SOS_OK)
	  {
	    return -EINVAL;
	  }
	// Kevin - resulting Matrix will be put unto updateBackground, and abssub   => 2 ports
	//PUT_PORT(); 
	//PUT_PORT(); 
	//  Kevin -  if these are all sos modules, shouldnt is use self state s->M ???
	// call imgBackground.updateBackground(&M, &backMat, &backMat);



	s->bckUpdate.matPtrA = M;
	s->bckUpdate.value = 1;	// Kevin -  the heck is this??


	//post_long(IMG_BACKGROUND_PID, MATRIX_IMAGE_PID, UPDATE_BACKGROUND, sizeof(background_update_t), s->bckUpdate, SOS_MSG_RELEASE);
	//post_long(MATRIX_ARITHMETICS_PID, MATRIX_IMAGE_PID, ABS_SUB, sizeof(matrix_math_t), M, SOS_MSG_RELEASE);

	break;
      }

// int8_t convertRGBToMatrix (CYCLOPS_Matrix * M, const CYCLOPS_Image * Im, uint8_t color)      
    case CONVERT_RGB_TO_MATRIX:
      {
	CYCLOPS_Matrix *M;
	RGB_to_matrix_t *m = (RGB_to_matrix_t *) (msg->data);
	CYCLOPS_Image *Im = (CYCLOPS_Image *) (m->ImgPtr);
	uint8_t color = m->color;

	M =
	  (CYCLOPS_Matrix *) ker_malloc (sizeof (CYCLOPS_Matrix),
					 MATRIX_IMAGE_PID);
	if (NULL == M)
	  {
	    return -ENOMEM;
	  }

	if (convertRGBToMatrix (M, Im, color) != SOS_OK)
	  {
	    return -EINVAL;
	  }

	//PUT_PORT();
	//post_long                                     
	break;
      }

    default:
      return -EINVAL;
    }

  return SOS_OK;
}

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


#ifndef _MODULE_
mod_header_ptr
matrix_image_get_header ()
{
  return sos_get_header_address (mod_header);
}
#endif
