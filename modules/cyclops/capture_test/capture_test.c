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
#include <sys_module.h>
#define LED_DEBUG
#include <led_dbg.h>

#include <hardware.h>
#include <image.h>
#include <adcm1700Control.h>
#include <adcm1700Const.h>
#include <camera_settings.h>

#ifdef USE_SERIAL_DUMP
#include <serialDump.h>
#else
#include <radioDump.h>
#endif

//-----------------------------------------------------
// CONSTANTS
//-----------------------------------------------------
#define CAPTURE_TEST_PID 			DFLT_APP_ID0
#define CAPTURE_TEST_TIMER_INTERVAL	4*1024L
#define CAPTURE_TEST_TID			0
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
typedef struct capture_test_state {
	uint8_t state;
	CYCLOPS_Image* img;
	CYCLOPS_Capture_Parameters* capture;
} capture_test_state_t;


//-----------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------
static int8_t capture_test_msg_handler(void *start, Message *e);
static int8_t init_capture_timer(void);
//-----------------------------------------------------
// MODULE HEADER
//-----------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = CAPTURE_TEST_PID,
  .state_size     = sizeof(capture_test_state_t),
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,
  .code_id        = ehtons(CAPTURE_TEST_PID),
  .module_handler = capture_test_msg_handler,
};

//-----------------------------------------------------
// MODULE HANDLER
//-----------------------------------------------------
static int8_t capture_test_msg_handler(void *state, Message *msg)
{
	capture_test_state_t *s = (capture_test_state_t*)state;

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
			sys_timer_stop(CAPTURE_TEST_TID);
			ker_releaseImagerClient(CAPTURE_TEST_PID);
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
					if (ker_registerImagerClient(CAPTURE_TEST_PID) != SOS_OK){
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

					s->state = IMAGER_PREPARED;	  

					// starts timer which sets off repeated captures (remove for one-shot case)
					init_capture_timer();
					return SOS_OK;
				}
		
				case CAPTURE_TEST_TID:
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
			/* The application is supposed to take the image data
			 * from the message, and free both the internal and
			 * external RAM storage after processing the image.
			 */
			LED_DBG(LED_YELLOW_TOGGLE);
			s->img = (CYCLOPS_Image *)sys_msg_take_data(msg);

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
			s->state = IMAGER_PREPARED;

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
  long time_per_photo = ((long)(_CYCLOPS_RESOLUTION_) * _CYCLOPS_RESOLUTION_ *
						 _CYCLOPS_COLOR_DEPTH_ / 8 * 3) + 2048; 
	//.375 is from experimentation

	sys_timer_start(CAPTURE_TEST_TID, time_per_photo, TIMER_REPEAT);

	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr capture_test_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

