/**
 * \file adcm1700Control.c
 * \brief Control Interface to the ADCM Imager
 */

//---------------------------------------------------------------
// INCLUDES
//---------------------------------------------------------------
#include <module.h>
#include <adcm1700Control.h>
#include <adcm1700Const.h>
#include <adcm1700Comm.h>
#include <adcm1700ctrlWindowSize.h>
#include <adcm1700ctrlStat.h>
#include <adcm1700ctrlRun.h>
#include <adcm1700ctrlPattern.h>
#include <adcm1700ctrlPatch.h>
#include <adcm1700ctrlFormat.h>
#include <adcm1700ctrlExposure.h>
#include <memConst.h>
#include <image.h>
#include <cpldConst.h>
#include <cpld.h>
#define LED_DEBUG
#include <led_dbg.h>

#include "pt.h"


//---------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------
#define CONTROL_TID   1 // Controller Timer Identifier

/**
 * Camera State Machine
 */
typedef enum 
  {
    CAMERA_NOT_POWERED	= 0x11,
    CAMERA_POWERED	= 0x23,
    CAMERA_CLOCKED	= 0x31,
    CAMERA_PATCHED      = 0x47,
    CAMERA_BUSY		= 0xFF,
    CAMERA_WARMING	= 0xFE,
    CAMERA_READY	= SOS_OK
  } camera_state_t;

/**
 * Camera commands
 */
typedef enum
  {
    CINIT,     //!< Initialization
    CSNAP,     //!< Snap Image
    CSTAT,     //!< Get Image Stats
  } camera_cmd_t;

//---------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//---------------------------------------------------------------
static int8_t adcm1700_control_handler (void *state, Message * e);
static int8_t initCamera(struct pt *ipt, Message* msg);
static int8_t snapImage(struct pt* spt, Message* msg);
static int8_t initCameraStatusCheck(Message* msg);
static int8_t snapImageStatusCheck(Message* msg);
static inline int8_t getSumsDone (color16_t values, uint8_t status);	// Ram - Status needs to change to int8_t
static inline void set_CPLD_run_mode (uint8_t opcode, uint8_t startPage, uint8_t count);


//---------------------------------------------------------------
// MODULE HEADER
//---------------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = ADCM1700_CONTROL_PID,
  .state_size     = 0,
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .module_handler = adcm1700_control_handler,
};

//---------------------------------------------------------------
// GLOBAL VARIABLES
//---------------------------------------------------------------

static camera_state_t controlState;
static camera_cmd_t currentCommand;
static sos_pid_t client_pid;


static struct pt ipt;
static struct pt spt;

static cpldCommand_t myCPLDcode;

static uint8_t sPage;		//start page for capture / test operations
static uint8_t nFrames;		// number of sequential frames to capture

// current image and capture parameters
static CYCLOPS_ImagePtr           pImg;	        //! Pointer to image structure
static CYCLOPS_Image              image;	//! Default image, used for init
static CYCLOPS_Capture_Parameters capture;	//! Capture info
static CYCLOPS_Image_result_t     imageResult;




//---------------------------------------------------------------
// MODULE HANDLER
//---------------------------------------------------------------

/**
 * Message handler for adcm1700 controller
 */

static int8_t adcm1700_control_handler (void *state, Message * msg)
{
  switch (msg->type){
    
  case MSG_INIT:
    {
      controlState = CAMERA_NOT_POWERED;
      TURN_CAMERA_POWER_OFF ();
      TURN_CAMERA_CLOCK_OFF ();
      // make sure camera Vcc goes off !!!
      // Set current status. Force the message handler to schedule the init thread
      currentCommand = CINIT;
      PT_INIT(&ipt);
      // Start a timer - Timeout message will begin the init thread
      ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID, TIMER_ONE_SHOT);
      ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 2000);    
      break;
    }

  case MSG_FINAL:
    {
      TURN_CAMERA_CLOCK_OFF ();	// it should ALREADY be off!
      TURN_CAMERA_POWER_OFF ();
      controlState = CAMERA_NOT_POWERED;
      // Kevin: component stops are not implemented...  
      break;
    }

  case SET_PATCH_DONE:
    initCamera(&ipt, msg);
    break;

  case MSG_TIMER_TIMEOUT:
  case SET_FORMAT_DONE:
  case SET_PATTERN_DONE:
  case SET_EXPOSURE_TIME_DONE:
  case SET_ANALOG_GAIN_DONE:
  case SET_DIGITAL_GAIN_DONE:
  case SET_INPUT_SIZE_DONE:
  case SET_OUTPUT_SIZE_DONE:
  case CAMERA_DONE:
    initCamera(&ipt, msg);
    snapImage(&spt, msg);
    break;


  case SET_INPUT_PAN_DONE:
  case SET_CPLD_MODE_DONE:
  case SENSOR_DONE:
    snapImage(&spt, msg);
    break;

  case SNAP_IMAGE_DONE:
    {
      /* This is a fix for the CPLD problem.
       * External memory has to be initialized
       * after the first image has been taken.
       */
      ext_mem_init();
      ker_releaseImagerClient(ADCM1700_CONTROL_PID);
    }

  case GET_SUMS_DONE:
    {
      color16_result_t *m = (color16_result_t *) (msg->data);
      color16_t values = m->values;
      uint8_t status = m->result;
      getSumsDone (values, status);
      break;
    }


  default:
    return -EINVAL;
  }
  return SOS_OK;
}


//---------------------------------------------------------------
// INIT CAMERA THREAD
//---------------------------------------------------------------
static int8_t initCamera(struct pt* ipt, Message* msg)
{
  // Cannot run this thread unless we are initializing
  if (CINIT != currentCommand) 
    return -EINVAL; 


  PT_BEGIN(ipt);

  PT_WAIT_UNTIL(ipt, msg->type == MSG_TIMER_TIMEOUT);
  // controlState should be CAMERA_NOT_POWERED
  // DC power should be provided for > 20 mS before MCLK is activated
  controlState = CAMERA_POWERED;
  TURN_CAMERA_POWER_ON ();

  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 100);
  PT_YIELD(ipt);
  PT_WAIT_UNTIL(ipt, msg->type == MSG_TIMER_TIMEOUT);
  // MCLK should be provided > 200 mS before the first I2C transaction
  controlState = CAMERA_CLOCKED;
  TURN_CAMERA_CLOCK_ON ();

  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 400);
  // Clock should run for 200 mS before I2C communication
  PT_YIELD(ipt);
  PT_WAIT_UNTIL(ipt, msg->type == MSG_TIMER_TIMEOUT);
  // end of MCLK on delay

  adcm1700CtrlPatchSetPatch ();
  PT_WAIT_UNTIL(ipt, msg->type == SET_PATCH_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  controlState = CAMERA_PATCHED;

  adcm1700CtrlFormatSetFormat (pImg->type);
  PT_WAIT_UNTIL(ipt, msg->type == SET_FORMAT_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  
  adcm1700CtrlSetPattern (capture.testMode);
  PT_WAIT_UNTIL(ipt, msg->type == SET_PATTERN_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);

  adcm1700CtrlSetExposureTime (capture.exposurePeriod);
  PT_WAIT_UNTIL(ipt, msg->type == SET_EXPOSURE_TIME_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  
  adcm1700CtrlSetAnalogGain (capture.analogGain);
  PT_WAIT_UNTIL(ipt, msg->type == SET_ANALOG_GAIN_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  
  adcm1700CtrlSetDigitalGain (capture.digitalGain);
  PT_WAIT_UNTIL(ipt, msg->type == SET_DIGITAL_GAIN_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  
  adcm1700WindowSizeSetInputSize (capture.inputSize);
  PT_WAIT_UNTIL(ipt, msg->type == SET_INPUT_SIZE_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  
  adcm1700WindowSizeSetOutputSize (pImg->size);
  PT_WAIT_UNTIL(ipt, msg->type == SET_OUTPUT_SIZE_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);

  adcm1700CtrlRunCamera (CYCLOPS_STOP);	// enter low-power standby mode
  PT_WAIT_UNTIL(ipt, msg->type == CAMERA_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);
  controlState = CAMERA_WARMING;
  LED_DBG (LED_GREEN_ON);

  // Run the imager for 5 seconds (for AE convergence)  
  // First run the imager
  adcm1700CtrlRunCamera (CYCLOPS_RUN);
  PT_YIELD(ipt);
  PT_WAIT_UNTIL(ipt, msg->type == CAMERA_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);

  // Then wait for 5 seconds  
  ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID, TIMER_ONE_SHOT);
  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 5000);
  PT_WAIT_UNTIL(ipt, msg->type == MSG_TIMER_TIMEOUT);

  adcm1700CtrlRunCamera (CYCLOPS_STOP);
  PT_WAIT_UNTIL(ipt, msg->type == CAMERA_DONE);
  if (initCameraStatusCheck(msg) != SOS_OK)
    PT_EXIT(ipt);

  controlState = CAMERA_READY;  
  /* Everything that we do from this point onwards
   * is to fix a hardware problem in Cyclops CPLD.
   * Initialize external memory after the first
   * image has been taken.
   */
  // This call will succeed
  ker_registerImagerClient(ADCM1700_CONTROL_PID);
  // Capture parameters have been properly initialized
  // Now snap an image
  CYCLOPS_Image pImg;
  pImg.type = CYCLOPS_IMAGE_TYPE_Y;
  // Multiple frame count
  pImg.nFrames = 1;
  // Output image size
  pImg.size.x = 128;
  pImg.size.y = 128;
  // Dummy value, later filled in properly by imager driver
  pImg.imageDataHandle = -1;
  ker_snapImage(&pImg);   

  PT_END(ipt);

  return SOS_OK;
}


static int8_t initCameraStatusCheck(Message* msg){
  MsgParam *p = (MsgParam *) (msg->data);
  uint8_t status = p->byte;
  if (status != SOS_OK)
    return -EINVAL;
  return SOS_OK;
}

//---------------------------------------------------------------
// SNAP IMAGE THREAD
//---------------------------------------------------------------
static int8_t snapImage(struct pt* spt, Message* msg)
{
  // Cannot run this thread unless we are snapping an image
  if (CSNAP != currentCommand) 
    return -EINVAL;   
  
  PT_BEGIN(spt);

  PT_WAIT_UNTIL(spt, msg->type == SET_FORMAT_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }
  
  adcm1700CtrlSetPattern (capture.testMode);
  PT_WAIT_UNTIL(spt, msg->type == SET_PATTERN_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  adcm1700CtrlSetExposureTime (capture.exposurePeriod);
  PT_WAIT_UNTIL(spt, msg->type == SET_EXPOSURE_TIME_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  adcm1700CtrlSetAnalogGain (capture.analogGain);
  PT_WAIT_UNTIL(spt, msg->type == SET_ANALOG_GAIN_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  adcm1700CtrlSetDigitalGain (capture.digitalGain);
  PT_WAIT_UNTIL(spt, msg->type == SET_DIGITAL_GAIN_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }
  
  adcm1700WindowSizeSetInputSize (capture.inputSize);
  PT_WAIT_UNTIL(spt, msg->type == SET_INPUT_SIZE_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  adcm1700WindowSizeSetOutputSize (pImg->size);
  PT_WAIT_UNTIL(spt, msg->type == SET_OUTPUT_SIZE_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }
  
  adcm1700CtrlRunCamera (CYCLOPS_RUN);	// start module running for capture
  PT_WAIT_UNTIL(spt, msg->type == CAMERA_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }
  
  adcm1700CtrlRunSensor (CYCLOPS_STOP);
  PT_WAIT_UNTIL(spt, msg->type == SENSOR_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }
  
  // perform input pan
  adcm1700WindowSizeSetInputPan (capture.offset);	
  PT_WAIT_UNTIL(spt, msg->type == SET_INPUT_PAN_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  adcm1700CtrlRunSensor (CYCLOPS_RUN);  
  PT_WAIT_UNTIL(spt, msg->type == SENSOR_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }


  if (capture.runTime >= 10) {
    // if the run time is too short, the event can be missed
    ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID, TIMER_ONE_SHOT);
    ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, capture.runTime);
  }
  else{
    // Ram - A hack to generate a dummy Timer Message
    // Need to fix it with a state variable
    msg->type = MSG_TIMER_TIMEOUT;
  }      
  PT_WAIT_UNTIL(spt, msg->type == MSG_TIMER_TIMEOUT);
  LED_DBG (LED_AMBER_TOGGLE);


  set_CPLD_run_mode (CPLD_OPCODE_CAPTURE_IMAGE, sPage, nFrames);    
  PT_WAIT_UNTIL(spt, msg->type == SET_CPLD_MODE_DONE);

  cpldCommand_t *cpldC = (cpldCommand_t *) (msg->data);
  if ((cpldC->opcode & 0x0f) == CPLD_OPCODE_CAPTURE_IMAGE)
    adcm1700CtrlRunCamera (CYCLOPS_STOP);
  else{
    PT_EXIT(spt);
  }
  PT_WAIT_UNTIL(spt, msg->type == CAMERA_DONE);
  if (snapImageStatusCheck(msg) != SOS_OK){
    PT_EXIT(spt);
  }

  // end of capture operation: CPLD is already in SRAM access mode
  pImg->result = SOS_OK;
  if (client_pid != NULL_PID) {
    post_long (client_pid, ADCM1700_CONTROL_PID, SNAP_IMAGE_DONE,
	       sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
  } else {
    ker_free_handle(pImg->imageDataHandle);
    ker_free(pImg);
  }
  controlState = CAMERA_READY;

  
  PT_END(spt);
  return SOS_OK;
}

static int8_t snapImageStatusCheck(Message* msg){
  MsgParam *p = (MsgParam *) (msg->data);
  uint8_t status = p->byte;
  if (status != SOS_OK){
    pImg->result = -EINVAL;
    post_long (client_pid, ADCM1700_CONTROL_PID, SNAP_IMAGE_DONE, 
	       sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
    controlState = CAMERA_READY;
    return -EINVAL;
  }
  return SOS_OK;
}


//---------------------------------------------------------------
// Ram - Status needs to change to int8_t
static inline int8_t getSumsDone (color16_t values, uint8_t status)	
{
  color16_t averages;
  color16_result_t averageResult;
  uint16_t scaleFactor;

  if (status != SOS_OK){
    averages.red = 0;
    averages.green = 0;
    averages.blue = 0;
    averageResult.values.red = averages.red;
    averageResult.values.green = averages.green;
    averageResult.values.blue = averages.blue;
    averageResult.result = -EINVAL;
    post_long (client_pid, ADCM1700_CONTROL_PID, GET_PIXEL_AVERAGES_DONE,
	       sizeof (averageResult), &averageResult, 0);
  }
  else{
    // The scale factor is 4*16384/Nx/Ny.
    // window size         scale factor
    //  280 * 280          0.86
    //  128 * 128          4
    //   64 *  64          16
    //   32 *  32          64
    // [note: minimum window size is 24 * 24]
    // scale factor * 128 (to avoid round-off errors)
    scaleFactor = 0x8000 / pImg->size.x;
    scaleFactor = (256 * scaleFactor) / pImg->size.y;
    averages.red = values.red * scaleFactor >> 7;
    averages.green = values.green * scaleFactor >> 7;
    averages.blue = values.blue * scaleFactor >> 7;
    averageResult.values.red = averages.red;
    averageResult.values.green = averages.green;
    averageResult.values.blue = averages.blue;
    averageResult.result = SOS_OK;
    post_long (client_pid, ADCM1700_CONTROL_PID, GET_PIXEL_AVERAGES_DONE,
	       sizeof (averageResult), &averageResult, 0);
  }
  return SOS_OK;
}

//---------------------------------------------------------------
// last argument used to be stopPage
static inline void set_CPLD_run_mode (uint8_t opcode, uint8_t startPage, uint8_t count)
{
  // A cleaner way to do this would be to change the definition of the
  // CPLDcommand structure. However it might have other consequences. 
  myCPLDcode.opcode = ((count & 0x0f) << 4) | (opcode & 0x0f);
  myCPLDcode.sramBank = 0x00;	//4 bit   0x00-0x0f
  myCPLDcode.flashBank = 0x00;	//4 bit   0x00-0x0f
  myCPLDcode.startPageAddress = startPage;	//1 byte  0x00-0xff
  myCPLDcode.endPageAddress = 0xFF;	//1 byte (ignored)
  setCpldMode (&myCPLDcode);
}

//---------------------------------------------------------------
// Get the average pixel values
uint8_t getPixelAverages ()
{
  currentCommand = CSTAT;
  adcm1700CtrlStatGetSums ();
  return SOS_OK;
}

//----------------------------------------------------------------------------------------------------------

/**
 * Initialize the adcm1700 driver and register with SOS kernel
 */
uint8_t adcm1700control_init ()
{
  client_pid = NULL_PID;
  // turn off Timer1 compare outputs
  cbi (TCCR1A, 7);
  cbi (TCCR1A, 6);
  cbi (TCCR1A, 5);
  cbi (TCCR1A, 4);		// *** DEBUG ***

  // Power on camera hardware (active low)
  //    CLR_MEM_CPLD_EN();
  //    SET_CPLD_CLOCK_EN();
  TURN_CAMERA_POWER_OFF ();	// the camera should already be off
  TURN_CAMERA_CLOCK_OFF ();	// the clock should already be off        

  controlState = CAMERA_NOT_POWERED;

  adcm1700ImagerCommInit ();
  adcm1700WindowSizeInit ();
  adcm1700CtrlRunInit ();
  adcm1700CtrlStatInit ();
  adcm1700CtrlPatternInit ();
  adcm1700CtrlPatchInit ();
  adcm1700CtrlFormatInit ();
  adcm1700CtrlExposureInit ();

  // default image parameters  (used during init)
  image.type = CYCLOPS_IMAGE_TYPE_Y;
  image.size.x = 0x40;		// 64x64 pixels        
  image.size.y = 0x40;
  image.nFrames = 0x01;
  pImg = &image;		// initialize pointer 

  // default capture parameters
  capture.offset.x = 0;
  capture.offset.y = 0;
  capture.inputSize.x = 0x120;
  capture.inputSize.y = 0x120;
  capture.testMode = CYCLOPS_TEST_MODE_NONE;	// *** DEBUG ***
  capture.exposurePeriod = 0x0;	// AE
  capture.analogGain.red = 0x00;
  capture.analogGain.green = 0x00;
  capture.analogGain.blue = 0x00;
  capture.digitalGain.red = 0x0000;	// AWB
  capture.digitalGain.green = 0x0000;
  capture.digitalGain.blue = 0x0000;
  capture.runTime = 500;

  ker_register_module (sos_get_header_address (mod_header));

  return SOS_OK;
}

//----------------------------------------------------------------------------------------------------------
// KERNEL API
//----------------------------------------------------------------------------------------------------------
// Register a client with the imager
int8_t ker_registerImagerClient (sos_pid_t pid)
{
  if (NULL_PID == pid) return -EINVAL;
  if (NULL_PID != client_pid) return -EBUSY;
  if (controlState != CAMERA_READY) return -EBUSY;
  client_pid = pid;
  return SOS_OK;
}
//----------------------------------------------------------------------------------------------------------
// Release a client with the imager
int8_t ker_releaseImagerClient (sos_pid_t pid)
{
  // A client can be released only when the imager is not busy
  if (controlState != CAMERA_READY) 
    return -EBUSY;
  
  if ((NULL_PID != pid) && (pid == client_pid)){
    client_pid = NULL_PID;
    return SOS_OK;
  }
  return -EINVAL;
}
//----------------------------------------------------------------------------------------------------------
// capture an image and store it in SRAM             
int8_t ker_snapImage (CYCLOPS_ImagePtr ptrImg)
{
  uint8_t* imageDataPtr;
  uint16_t imgSize;

  LED_DBG (LED_YELLOW_TOGGLE);
  if (controlState != CAMERA_READY){	
    // in case the init routine -EINVALed ...
    imageResult.ImgPtr = NULL;
    imageResult.result = -EINVAL;
    post_long (client_pid, ADCM1700_CONTROL_PID, SNAP_IMAGE_DONE, sizeof (imageResult), &imageResult, 0);
    return -EINVAL;
  }

  pImg = (CYCLOPS_Image*)ker_malloc(sizeof(CYCLOPS_Image), ADCM1700_CONTROL_PID);
  if (NULL == pImg) {
    return -ENOMEM;
  }
  memcpy(pImg, ptrImg, sizeof(CYCLOPS_Image));
  pImg->result = SOS_OK;

  imgSize = imageSize(pImg);
  pImg->imageDataHandle = ker_get_handle(imgSize, ADCM1700_CONTROL_PID);
  if (pImg->imageDataHandle < 0) {
    ker_free(pImg);
    return -EINVAL;
  }
		
  imageDataPtr = (uint8_t*) ker_get_handle_ptr(pImg->imageDataHandle);
  if (NULL == imageDataPtr) {
    return -EINVAL;
  }
  sPage = (uint8_t) (((uint16_t) (imageDataPtr)) >> 8);	//divide by 256 to get the page number
  nFrames = (pImg->nFrames & 0x0F);	// limit capture to 15 frames

  // Start the Snap Image Thread
  currentCommand = CSNAP;
  PT_INIT(&spt);
  controlState = CAMERA_BUSY;
  adcm1700CtrlFormatSetFormat (pImg->type);	// jump into the rabbit hole!

  return SOS_OK;
}
//----------------------------------------------------------------------------------------------------------
// set new capture parameters
// (These values are not communicated to the module until the next capture is performed)
int8_t ker_setCaptureParameters (CYCLOPS_CapturePtr pCap)
{
  // update all capture parameters
  // (value checking is performed inside the individual components)
  capture.offset.x = pCap->offset.x;
  capture.offset.y = pCap->offset.y;
  capture.inputSize.x = pCap->inputSize.x;
  capture.inputSize.y = pCap->inputSize.y;
  capture.testMode = pCap->testMode;
  capture.exposurePeriod = pCap->exposurePeriod;
  capture.analogGain.red = pCap->analogGain.red;	// zero for auto exposure mode
  capture.analogGain.green = pCap->analogGain.green;
  capture.analogGain.blue = pCap->analogGain.blue;
  capture.digitalGain.red = pCap->digitalGain.red;	// zero for AWB mode    
  capture.digitalGain.green = pCap->digitalGain.green;
  capture.digitalGain.blue = pCap->digitalGain.blue;
  capture.runTime = pCap->runTime;
  return SOS_OK;
}
