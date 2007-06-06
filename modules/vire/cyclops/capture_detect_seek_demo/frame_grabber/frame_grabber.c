/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Frame Grabber  Module
 * 
 * Extension of object detect application which plus zooming on a detected object
 * 
 * \author Kevin Lee
 * \author Rahul Balani
 */  

//-----------------------------------------------------
// INCLUDES
//-----------------------------------------------------
#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
#define LED_DEBUG
#include <led_dbg.h>

/*
 * Platform/Application specific header files.
 */
#include <hardware.h>
#include <matrix.h>
#include <adcm1700Control.h>
#include <adcm1700Const.h>
#include "../include/app_config.h"

//-----------------------------------------------------
// CONSTANTS
//-----------------------------------------------------
#define FRAME_GRABBER_PID		DFLT_APP_ID0
#define FRAME_GRABBER_TID1		1
#define DEFAULT_SAMPLE_RATE		10*1024L
#define MIN_SAMPLE_RATE			100L

/**
 * Valid states for this application to be in
 */
enum {
	IMAGER_IDLE				= 0,
	IMAGER_CAPTURE_PENDING	= 1,
	IMAGER_UNREGISTERED		= 2,
	IMAGER_STALLED			= 3,
};

//-----------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------
typedef struct frame_grabber_state {
	func_cb_ptr output0;		//!< Output port for regular periodic images.
	func_cb_ptr output1;		//!< Output port for special images.
	//func_cb_ptr put_token;		
	func_cb_ptr signal_error;
	uint32_t sampleRate;		//!< Exposed parameter. Sample rate for taking pictures.
	uint8_t zoomIn;				//!< Exposed parameter. Controls whether camera should zoom in
								//!< after an object has been detected.
	// Internal state variables.
	sos_pid_t pid;
	uint8_t state;
	uint8_t actionFlag;
	CYCLOPS_Image *img;    
	CYCLOPS_Capture_Parameters *capture; 
	CYCLOPS_Matrix *M;

  //uint8_t objectDetected;
  //uint8_t framesize;
} frame_grabber_state_t;


//-----------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------
static int8_t frame_grabber_msg_handler(void *start, Message *e);
static void resetCaptureParameters(CYCLOPS_Capture_Parameters *capture );
static int8_t takePicture(frame_grabber_state_t *s);
static void destroy_image(CYCLOPS_Image *img);

//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length);
static int8_t input0 (func_cb_ptr p, token_type_t *t);
static int8_t update_param (func_cb_ptr p, void *data, uint16_t length);

//-----------------------------------------------------
// MODULE HEADER
//-----------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  	.mod_id         = FRAME_GRABBER_PID,
  	.state_size     = sizeof(frame_grabber_state_t),
	.num_out_port   = 2,
  	.num_sub_func   = 3,
  	.num_prov_func  = 2,
 	.platform_type  = HW_TYPE,
  	.processor_type = MCU_TYPE,
  	.code_id        = ehtons(FRAME_GRABBER_PID),
  	.module_handler = frame_grabber_msg_handler,	
  	.funct			= {
  		{error_8, "cyC2", FRAME_GRABBER_PID, INVALID_GID},
  		{error_8, "cyC2", FRAME_GRABBER_PID, INVALID_GID},
		//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
		{error_8, "ccv1", MULTICAST_SERV_PID, SIGNAL_ERR_FID},
        {input0, "cyC2", FRAME_GRABBER_PID, INPUT_PORT_0},
		{update_param, "cwS2", FRAME_GRABBER_PID, UPDATE_PARAM_FID},
  	},
};

//-----------------------------------------------------
// MODULE HANDLER
//-----------------------------------------------------

/** Main message handler for frame grabber module
 *
 * \param state State of the module describing the current state of the image capture process
 *
 * \param msg Incoming message
 *
 * This kernel modules handles the following messages that are probably
 * generated by drivers:
 *
 * \li SNAP_IMAGE_DONE Signal from the driver level when the image snap procedure completes
 * the image data pointer points to the resulting image.
 *
 */
static int8_t frame_grabber_msg_handler(void *state, Message *msg) {
  	frame_grabber_state_t *s = (frame_grabber_state_t*)state;

	switch (msg->type){    
		/**
		 * \par MSG_INIT
		 * Initialize all necessary parameters for image capture as well as registering the application with the cyclops driver
		 */
		case MSG_INIT:
		{
			LED_DBG(LED_YELLOW_ON);
			LED_DBG(LED_GREEN_ON);
			LED_DBG(LED_AMBER_ON);
			// Set the default values of exposed parameters
			s->sampleRate = DEFAULT_SAMPLE_RATE;
			s->zoomIn = 1;

			// Set internal state
			s->pid = msg->did;
			s->actionFlag = 0;
			// Allocate Dynamic Memory
			s->capture = (CYCLOPS_Capture_Parameters*) 
								vire_malloc(sizeof(CYCLOPS_Capture_Parameters), s->pid);
			if (NULL == s->capture){ 
				return -ENOMEM;
			}

			s->M = (CYCLOPS_Matrix*) vire_malloc(sizeof(CYCLOPS_Matrix), s->pid);
			if (s->M == NULL) {
				vire_free(s->capture, sizeof(CYCLOPS_Capture_Parameters));
				return  -ENOMEM;
			}		
			s->M->rows = ROWS;
			s->M->cols = COLS;
			s->M->depth= CYCLOPS_1BYTE;
			s->M->data.hdl8 = -1;

			// Register as an Imager Client
			if (ker_registerImagerClient(s->pid) != SOS_OK){
				// Ram - No recovery currently implemented
				vire_free(s->capture, sizeof(CYCLOPS_Capture_Parameters));
				vire_free(s->M, sizeof(CYCLOPS_Matrix));
				s->state = IMAGER_UNREGISTERED;
				return -EPERM;
			}

			// Setup the image data structure
			//s->framesize = _CYCLOPS_RESOLUTION_;			
			s->state = IMAGER_IDLE;		
			s->img = NULL;

			// Setup the capture parameters
			resetCaptureParameters(s->capture);
			ker_setCaptureParameters(s->capture);

			sys_timer_start(FRAME_GRABBER_TID1, s->sampleRate, TIMER_ONE_SHOT);		
			DEBUG("frame_grabber Start\n");
			break;
		}

		/**
		 * \par MSG_FINAL
		 * Clean up itself when done
		 */
		case MSG_FINAL:
		{
			ker_releaseImagerClient(s->pid);
			sys_timer_stop(FRAME_GRABBER_TID1);
			vire_free(s->capture, sizeof(CYCLOPS_Capture_Parameters));
			vire_free(s->M, sizeof(CYCLOPS_Matrix));
			destroy_image(s->img);
			DEBUG("frame_grabber Stop\n");
			break;
		}

		/**
		 * \par MSG_TIMER_TIMEOUT
		 * Timer fired, snap regular image
		 */    
		case MSG_TIMER_TIMEOUT:
		{				
			LED_DBG(LED_AMBER_TOGGLE);
			sys_timer_start(FRAME_GRABBER_TID1, s->sampleRate, TIMER_ONE_SHOT);		
			return takePicture(s);
		}
	
		/** 
		 * \par SNAP_IMAGE_DONE
		 */
		case SNAP_IMAGE_DONE:
		{
		  // start obj det red on here
			s->img = (CYCLOPS_Image *)sys_msg_take_data(msg);
			s->state = IMAGER_IDLE;
			LED_DBG(LED_YELLOW_TOGGLE);
			switch (s->actionFlag) {
				case NONE:
				{
					token_type_t *my_token;


					if (s->img->result < 0) break;

					if (convertImageToMatrix(s->M, s->img) < 0) break;
					my_token = create_token(s->M, sizeof(CYCLOPS_Matrix), s->pid);
					if (my_token == NULL) return -ENOMEM;
					set_token_type(my_token, CYCLOPS_MATRIX);
					// Can not release matrix here as it shares external RAM with
					// s->img. It is released with the image later.
					//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token); 
					dispatch(s->output0, my_token); 
					destroy_token(my_token);

					// Placing token on output 0 is supposed to set the
					// actionFlag according to feedback obtained.
					//if (s->actionFlag == NONE) {
						// Just put a NULL token on output 1.
						//destroy_image(s->img);
						//s->img = NULL;
						// Let it fall through the next case to put the NULL token.
					//} else if (s->actionFlag == OBJ_DETECT) {
					if (s->actionFlag == OBJ_DETECT) {
						if (s->zoomIn) {
							if (takePicture(s) == 0) {
								// Break out of here to get zoomed picture later.
								break;
							} 
							// Couldn't zoom in to take picture.
							// Put original picture on output port 1.
							// Let it fall through the next case.
						}
						// Let it fall through to send the original picture on port 2
						// if it doesn't have to take a zoomed picture.
					} 
					//else {
						// Some unknown flag. Ignore.
						//break;
					//}
				}
				case OBJ_DETECT:
				{
					// start here for capture_test red
					token_type_t *my_token = NULL;
					if (s->img->result < 0) {
						destroy_image(s->img);
						s->img = NULL;
					}
					// Got the zoomed picture. Put it on output 1.
					if (s->img != NULL) {
						my_token = create_token(s->img, sizeof(CYCLOPS_Image), s->pid);
					} else {
						my_token = create_token(s->img, 0, s->pid);
					}
					if (my_token == NULL) return -ENOMEM;
					set_token_type(my_token, CYCLOPS_IMAGE);
					release_token(my_token);
					//SOS_CALL(s->put_token, put_token_func_t, s->output1, my_token);
					dispatch(s->output1, my_token);
					destroy_token(my_token);

					// Reset capture parameters and actionFlag
					s->actionFlag = NONE;
					s->img = NULL;
					resetCaptureParameters(s->capture);
					ker_setCaptureParameters(s->capture);

					return SOS_OK;
				}
				default: break;
			}

			// Image is not required any more. Free it.
			// It is reached only when iamge output from camera is invalid,
			// or when convertImageToMatrix fails,
			// or zoomed picture has been set to be taken.
			destroy_image(s->img);
			s->img = NULL;

			break;
		}
	
		default: 
		{
			return -EINVAL;
		}
	}

	return SOS_OK;
}
//-----------------------------------------------------
// RESET THE CAPTURE PARAMETER
//-----------------------------------------------------
/**
 * Set the capture parameter to default values
 *
 * \param capture Pointer to the data structure containing capture parameters
 *
 * \return none
 */
static void resetCaptureParameters(CYCLOPS_Capture_Parameters *capture )
{
	// Setup the capture parameters
	capture->offset.x           = 0;
	capture->offset.y           = 0;
	capture->inputSize.x        = 0x120;
	capture->inputSize.y        = 0x120;
	capture->testMode           = CYCLOPS_TEST_MODE_NONE;
	capture->exposurePeriod     = 0x0;// AE
	capture->analogGain.red     = 0x00;
	capture->analogGain.green   = 0x00;
	capture->analogGain.blue    = 0x00;
	capture->digitalGain.red    = 0x0000;// AWB
	capture->digitalGain.green  = 0x0000;  
	capture->digitalGain.blue   = 0x0000;  
	capture->runTime            = 500;// required to enable AE
	return;
}
//-----------------------------------------------------	
static int8_t takePicture(frame_grabber_state_t *s) 
{
	/* Initialize a CYCLOPS_Image structure according to
	 * the desired picture characteristics set in 
	 * camera_settings.h file.
	 */
	CYCLOPS_Image pImg;

	if (s->state != IMAGER_IDLE) return -EINVAL;

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
		s->state = IMAGER_IDLE;
		return -ENOMEM;
	}		

	return SOS_OK;
}

static void destroy_image(CYCLOPS_Image *img) {
	if (img == NULL) return;
	ker_free_handle(img->imageDataHandle);
	vire_free(img, sizeof(CYCLOPS_Image));
}

//-----------------------------------------------------	
// INPUT PORTS
//-----------------------------------------------------	
/**
 * Port handler for setting x and y variables in capture parameters
 *
 * \param data X and Y coordinates
 *
 * \param length
 *
 * \return SOS_OK if all goes well
 */
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	frame_grabber_state_t *s = (frame_grabber_state_t*)sys_get_state();
	objectInfo *temp = (objectInfo*)get_token_data(t);	

	s->actionFlag = temp->actionFlag;
	if ((temp->actionFlag == OBJ_DETECT) && (s->zoomIn)) {
		s->capture->offset.x = -(s->capture->inputSize.x/2) + (temp->objectPosition.x * s->capture->inputSize.x / s->img->size.x);
		s->capture->offset.y = -(s->capture->inputSize.y/2) + (temp->objectPosition.y * s->capture->inputSize.y / s->img->size.y);	
		s->capture->inputSize.x = temp->objectSize.x;
		s->capture->inputSize.y = temp->objectSize.y;
		ker_setCaptureParameters(s->capture);
	}	
	return SOS_OK;
}
//-----------------------------------------------------	

// Parameter updates
static int8_t update_param(func_cb_ptr p, void *data, uint16_t length) {
  	frame_grabber_state_t *s = (frame_grabber_state_t *)sys_get_state();
  	s->sampleRate = ehtonl(*((uint32_t *)data));
	s->zoomIn = *((uint8_t *)(data + sizeof(uint32_t)));
  	return SOS_OK;
}

//-----------------------------------------------------
#ifndef _MODULE_
mod_header_ptr frame_grabber_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


