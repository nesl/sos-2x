/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @author: Kevin Lee 1/17/05
 * @brief cpld test driver for cyclops
 * This should be identical to the mote side and it can be merged togther in the future.
 */

//-----------------------------------------------------
// INCLUDES
//-----------------------------------------------------

#include <hardware.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <sys_module.h>
#include <string.h>
#include <image.h>
#include <adcm1700Control.h>
#include <matrix.h>
#include <basicStat.h>
#include <imgBackground.h>
#include <camera_settings.h>
#include "object_detection.h"

#ifdef USE_SERIAL_DUMP
#include <serialDump.h>
#else
#include <radioDump.h>
#endif


//-----------------------------------------------------
// CONSTANTS
//-----------------------------------------------------
#define OBJECT_DETECTION_PID 			DFLT_APP_ID0
#define OBJECT_DETECTION_TIMER_INTERVAL	4*1024L
#define OBJECT_DETECTION_TID			0
#define INIT_WAIT_TIMER_INTERVAL 	10*1024L
#define INIT_WAIT_TID				1

enum {
  IMAGER_WAIT_FOR_INIT 	= 0,
  IMAGER_PREPARED,
  IMAGER_ERROR,
  IMAGER_CAPTURE_PENDING,
};

//-----------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------
typedef struct object_detection_state {
  uint8_t state;
  CYCLOPS_Image* img;
  CYCLOPS_Capture_Parameters* capture;
  uint8_t firstImage;
  //Matrix components
  CYCLOPS_Matrix *M;		//Matrix for image that was just grabbed
  CYCLOPS_Matrix *backMat;	//Matrix for storing the background
  CYCLOPS_Matrix *objMat;	//Matrix that stores the foreground
} object_detection_state_t;


//-----------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------
static int8_t object_detection_msg_handler(void *start, Message *e);
static int8_t init_capture_timer(void);
static CYCLOPS_Matrix *create_cyclops_matrix ();
static void destroy_cyclops_matrix(CYCLOPS_Matrix *m);
static void destroy_image(CYCLOPS_Image *img);
//-----------------------------------------------------
// MODULE HEADER
//-----------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = OBJECT_DETECTION_PID,
  .state_size     = sizeof(object_detection_state_t),
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,
  .code_id        = ehtons(OBJECT_DETECTION_PID),
  .module_handler = object_detection_msg_handler,
};

//-----------------------------------------------------
// MODULE HANDLER
//-----------------------------------------------------
static int8_t object_detection_msg_handler(void *state, Message *msg)
{
  object_detection_state_t *s = (object_detection_state_t*)state;

  switch (msg->type){
  case MSG_INIT:
    {
      LED_DBG(LED_GREEN_ON);
      LED_DBG(LED_YELLOW_ON);
      LED_DBG(LED_AMBER_ON);

      // Allocate Dynamic Memory
      s->capture = (CYCLOPS_Capture_Parameters*)
		sys_malloc(sizeof(CYCLOPS_Capture_Parameters));
      if (s->capture == NULL) {
		return -ENOMEM;
      }

      // Setup the image data structure
      s->state = IMAGER_WAIT_FOR_INIT;

      s->img = NULL;

      s->backMat = create_cyclops_matrix ();
      if (NULL == s->backMat) {
		sys_free(s->capture);
		return -ENOMEM;
      }	  
      s->objMat = create_cyclops_matrix ();
      if (NULL == s->objMat){
		sys_free(s->capture);
		destroy_cyclops_matrix(s->backMat);
		s->backMat = NULL;
		return -ENOMEM;
      }
      
      // Instead of calling create_cyclops_matrix() allocate spaces 
      // manually so the handle will be copied from the CYCLOPS_Image
      s->M = (CYCLOPS_Matrix *) sys_malloc (sizeof (CYCLOPS_Matrix));
      if (NULL == (s->M)) {
		sys_free(s->capture);
		destroy_cyclops_matrix(s->backMat);
		s->backMat = NULL;
		destroy_cyclops_matrix(s->objMat);
		s->objMat = NULL;
		return -ENOMEM;
      }
      
      s->M->rows = ROWS;
      s->M->cols = COLS;
      s->M->depth = CYCLOPS_1BYTE;
      s->M->data.hdl8 = 0xFF;
      
      // Setup the capture parameters
      s->capture->offset.x           = 0;
      s->capture->offset.y           = 0;
      s->capture->inputSize.x        = 0x120;
      s->capture->inputSize.y        = 0x120;
      s->capture->testMode           = CYCLOPS_TEST_MODE_NONE;
      s->capture->exposurePeriod     = 0x0;    // AE
      s->capture->analogGain.red     = 0x00;
      s->capture->analogGain.green   = 0x00;
      s->capture->analogGain.blue    = 0x00;
      s->capture->digitalGain.red    = 0x0000;   // AWB
      s->capture->digitalGain.green  = 0x0000;  
      s->capture->digitalGain.blue   = 0x0000;  
      s->capture->runTime            = 500;      // required to enable AE

      sys_timer_start(INIT_WAIT_TID, INIT_WAIT_TIMER_INTERVAL, TIMER_ONE_SHOT);

      break;
    }
	
  case MSG_FINAL:
    {
      sys_timer_stop(OBJECT_DETECTION_TID);
      ker_releaseImagerClient(OBJECT_DETECTION_PID);
      sys_free(s->capture);
	  destroy_cyclops_matrix(s->backMat);
	  destroy_cyclops_matrix(s->objMat);
      sys_free(s->M);
      DEBUG ("object_detection Stop\n");
      break;
    }
    //----------------------
    // TIMER HANDLER
    //----------------------
  case MSG_TIMER_TIMEOUT:
    {
      MsgParam *param = (MsgParam*) (msg->data);
      switch (param->byte){
      case INIT_WAIT_TID:
		{
		  // finish initialization
		  // Register as an Imager Client
		  if (ker_registerImagerClient(OBJECT_DETECTION_PID) != SOS_OK) {
			/* If -EBUSY is returned, it means either some other client
			 * is using the imager, or the imager is not initialized
			 * completely, or it is busy taking a picture
			 * initiated by the last client.
			 *
			 * So try again.
			 */
			s->state = IMAGER_ERROR;
			return -EPERM;
		  }

		  ker_setCaptureParameters(s->capture);
		  s->state = IMAGER_PREPARED;	  

		  // starts timer which sets off repeated captures (remove for one-shot case)
		  init_capture_timer();
	  
		  return SOS_OK;
		}
		
      case OBJECT_DETECTION_TID:
		{
		  /* Initialize a CYCLOPS_Image structure according to
		   * the desired picture characteristics set in 
		   * camera_settings.h file.
		   */
		  CYCLOPS_Image pImg;

		  if (IMAGER_PREPARED != s->state){
			return -EINVAL;
		  }

#if _CYCLOPS_COLOR_DEPTH_ == 3
		  pImg.type = CYCLOPS_IMAGE_TYPE_RGB;
#elif _CYCLOPS_COLOR_DEPTH_ == 2
		  pImg.type = CYCLOPS_IMAGE_TYPE_YCbCr;
#else
		  pImg.type = CYCLOPS_IMAGE_TYPE_Y;
#endif

		  // Multiple frame count
		  pImg.nFrames = _CYCLOPS_IMAGE_NFRAMES_;

		  // Output image size
		  pImg.size.x = _CYCLOPS_RESOLUTION_H;
		  pImg.size.y = _CYCLOPS_RESOLUTION_W;

		  // Dummy value, later filled in properly by imager driver
		  pImg.imageDataHandle = -1;

		  s->state = IMAGER_CAPTURE_PENDING;

		  if (ker_snapImage(&pImg) < 0) {
			/* Either the node is out of memory, or the imager is
			 * already busy taking a picture.
			 */
			s->state = IMAGER_ERROR;
			return -ENOMEM;
		  }
		  break; 
		}
      }

      break;
    }  
    //----------------------
    // SNAP IMAGE HANDLER
    //----------------------
  case SNAP_IMAGE_DONE:
    {
      double bckAvg; //background average
      uint8_t threshold, maxrow, maxcol, maxvalue, overTheThresh;

      LED_DBG(LED_YELLOW_TOGGLE);
      LED_DBG(LED_RED_TOGGLE);
      s->img = (CYCLOPS_Image *)sys_msg_take_data(msg);
	  s->state = IMAGER_PREPARED;
      convertImageToMatrix(s->M, s->img);

      if (s->firstImage == 1) {	//first image collected, backMat should equal image	
		uint8_t *back_ptr8;
		uint8_t *obj_ptr8;
		uint8_t *M_ptr8;
	
		obj_ptr8 = ker_get_handle_ptr (s->objMat->data.hdl8);
		back_ptr8 = ker_get_handle_ptr (s->backMat->data.hdl8);
		M_ptr8 = ker_get_handle_ptr (s->M->data.hdl8);
	
		if (obj_ptr8 == NULL) return -EINVAL;
		if (back_ptr8 == NULL) return -EINVAL;
		if (M_ptr8 == NULL) return -EINVAL;

		memcpy (back_ptr8, M_ptr8, s->M->rows * s->M->cols);
		memcpy (obj_ptr8, M_ptr8, s->M->rows * s->M->cols);
		s->firstImage = 0;
		destroy_image(s->img);
      } else { 	// This is not the first image. Process completely.
		uint8_t *obj_ptr8;
		obj_ptr8 = ker_get_handle_ptr (s->objMat->data.hdl8);
		if (obj_ptr8 == NULL) return -EINVAL;
	
		updateBackground (s->M, s->backMat, COEFFICIENT);
		abssub (s->M, s->backMat, s->objMat);	//foreground                           
		bckAvg = estimateAvgBackground (s->backMat, SKIP);	//illumination
		threshold = (uint8_t) (bckAvg * COEFFICIENT);
		maxvalue = maxLocate (s->objMat, &maxrow, &maxcol);	//locate max
		overTheThresh = OverThresh (s->objMat, maxrow, maxcol, RANGE, threshold);
		if (overTheThresh > DETECT_THRESH) {
#ifdef USE_SERIAL_DUMP
		  if (sys_post(SERIAL_DUMP_PID, MSG_DUMP_BUFFER_TO_SERIAL, sizeof(CYCLOPS_Image), 
					   s->img, SOS_MSG_RELEASE) != SOS_OK) {
			LED_DBG(LED_RED_TOGGLE);
		  }
#else
		  if (sys_post(RADIO_DUMP_PID, MSG_DUMP_BUFFER_TO_RADIO, sizeof(CYCLOPS_Image), 
					   s->img, SOS_MSG_RELEASE) != SOS_OK) {
			LED_DBG(LED_RED_TOGGLE);
		  }
#endif		  
		} else {
			destroy_image(s->img);
		}
      }
	  s->img = NULL;
      break;
    }
	
  default:
    return -EINVAL;
  }

  return SOS_OK;
}

//-----------------------------------------------------
// SETUP CAPTURE TIMER
//-----------------------------------------------------
static int8_t init_capture_timer(void)
{
  //this is the fastest you can take photos on a micaz currently.
  long time_per_photo = 2048 + ((long)(_CYCLOPS_RESOLUTION_) * _CYCLOPS_RESOLUTION_ *
								_CYCLOPS_COLOR_DEPTH_ / 8 * 3); 
  //.375 is from experimentation

  sys_timer_start(OBJECT_DETECTION_TID, time_per_photo, TIMER_REPEAT);

  return SOS_OK;
}

//-----------------------------------------------------
// ALLOCATE A MATRIX
//-----------------------------------------------------
/**
 * Allocate enough space for the matrix
 *
 * \param none 
 *
 * \return  pointer to the data if all goes well
 */
static CYCLOPS_Matrix * create_cyclops_matrix ()
{
  CYCLOPS_Matrix *pMat;
  uint16_t matrixSize;

  pMat = (CYCLOPS_Matrix *) sys_malloc (sizeof (CYCLOPS_Matrix));
  if (NULL == pMat)
	{
	  return NULL;
	}
  pMat->rows = ROWS;
  pMat->cols = COLS;
  pMat->depth = CYCLOPS_1BYTE;

  matrixSize = (pMat->rows) * (pMat->cols) * (pMat->depth);
  pMat->data.hdl8 = ker_get_handle (matrixSize, OBJECT_DETECTION_PID);
  if (pMat->data.hdl8 < 0)
	{
	  sys_free (pMat);
	  return NULL;
	}
  return pMat;
}

static void destroy_cyclops_matrix(CYCLOPS_Matrix *m) {
	if (m == NULL) return;
	ker_free_handle(m->data.hdl8);
	sys_free(m);
}

static void destroy_image(CYCLOPS_Image *img) {
	if (img == NULL) return;
	ker_free_handle(img->imageDataHandle);
	sys_free(img);
}

#ifndef _MODULE_
mod_header_ptr object_detection_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

