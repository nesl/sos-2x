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
//#define LED_DEBUG
#include <led_dbg.h>

//---------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------
#define CONTROL_TID   1 // Controller Timer Identifier

/**
 * Camera State Machine
 */
enum camera_state_t
  {
    //ON OFF ProcedureADDRESS
    CAMERA_NOT_POWERED	= 0x11,
    CAMERA_POWERED	= 0x23,
    CAMERA_CLOCKED	= 0x31,
    CAMERA_PATCHED      = 0x47,
    CAMERA_BUSY		= 0xFF,
    CAMERA_WARMING	= 0xFE,
    CAMERA_READY	= SOS_OK
  };

/**
 * Camera commands
 */
typedef enum
  {
    CINIT,     //!< Initialization
    CSET,      //!< Set Capture Parameters
    CSNAP,     //!< Snap Image
    CSTAT,     //!< Get Image Stats
    CRUN       //!< Run Module (for autofunction convergence)
  } camera_cmd_t;

//---------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//---------------------------------------------------------------
static int8_t adcm1700_control_handler (void *state, Message * e);
static int8_t adcm1700_control_timer_handler ();
static int8_t getSumsDone (color16_t values, uint8_t status);	// Ram - Status needs to change to int8_t
static void set_CPLD_run_mode (uint8_t opcode, uint8_t startPage, uint8_t count);
static int8_t adcm1700control_run (uint16_t rTime);	// run the imager for the specified period
static int8_t adcm1700_control_camera_handler (Message * msg);
static int8_t adcm1700_control_output_size_handler (Message * msg);

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
// state of this interface (which command was called, and how far as it progressed)
static uint8_t controlState;
static uint8_t client_pid;
static cpldCommand_t myCPLDcode;
static camera_cmd_t currentCommand;
static uint8_t sPage;		//start page for capture / test operations
static uint8_t nFrames;		// number of sequential frames to capture
// current image and capture parameters
static CYCLOPS_ImagePtr pImg;	// pointer to image structure
static CYCLOPS_Image image;	// default image, used for init
static CYCLOPS_Capture_Parameters capture;	// capture info
static CYCLOPS_Image_result_t imageResult;
static uint16_t runTime;	// run time (milliseconds) for run command
static uint8_t snapCount;

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
      snapCount = 0;
      currentCommand = CINIT;
      controlState = CAMERA_NOT_POWERED;
      TURN_CAMERA_POWER_OFF ();
      TURN_CAMERA_CLOCK_OFF ();
      // make sure camera Vcc goes off !!!
      // Ram - The imager is initialized only when a client registers with it (ker_registerImagerClient)
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

  case MSG_TIMER_TIMEOUT:
    {
      adcm1700_control_timer_handler ();
      break;
    }

  case SET_CPLD_MODE_DONE:
    {
      cpldCommand_t *cpldC = (cpldCommand_t *) (msg->data);
      switch (cpldC->opcode & 0x0f)	// RLB 7/21/05 masked out repeat field
	{
	case CPLD_OPCODE_CAPTURE_IMAGE:	// capture is complete, restore memory access
	  adcm1700CtrlRunCamera (CYCLOPS_STOP);
	  break;
	default:		// shouldn't ever get here
	  break;
	}
      return SOS_OK;
    }

  case SET_PATCH_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  //  Signal failure to initialize to the upper layers
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL, 0,
			0);
	  return -EINVAL;
	}
      controlState = CAMERA_PATCHED;
      adcm1700CtrlFormatSetFormat (pImg->type);
      return SOS_OK;
    }
    
  case SET_FORMAT_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700CtrlSetPattern (capture.testMode);
      return SOS_OK;
    }

  case SET_PATTERN_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700CtrlSetExposureTime (capture.exposurePeriod);
      return SOS_OK;
    }

  case SET_EXPOSURE_TIME_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700CtrlSetAnalogGain (capture.analogGain);
      return SOS_OK;
    }

  case SET_ANALOG_GAIN_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700CtrlSetDigitalGain (capture.digitalGain);
      return SOS_OK;
    }

    // set digital gain
  case SET_DIGITAL_GAIN_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700WindowSizeSetInputSize (capture.inputSize);
      return SOS_OK;
    }

    // input window size
  case SET_INPUT_SIZE_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  switch (currentCommand)
	    {
	    case CINIT:
	      if (client_pid != NULL_PID)
		post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			    0, 0);
	      return -EINVAL;
	      break;
	    case CSNAP:
	      pImg->result = -EINVAL;
	      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
			 sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	      //pImg = NULL;
	      controlState = CAMERA_READY;
	      return -EINVAL;
	      break;
	    default:
	      break;
	    }
	}
      adcm1700WindowSizeSetOutputSize (pImg->size);
      return SOS_OK;
    }

    // output window size
  case SET_OUTPUT_SIZE_DONE:
    {
      adcm1700_control_output_size_handler (msg);
      break;
    }
    //Run module
  case CAMERA_DONE:
    {
      adcm1700_control_camera_handler (msg);
      break;
    }
    // run sensor
  case SENSOR_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      //p->word is 16 bit, should i typecast?
      uint8_t run_stop = p->word;
      if (status != SOS_OK)
	{
	  pImg->result = -EINVAL;
	  post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		     sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	  //pImg = NULL;
	  controlState = CAMERA_READY;
	  return -EINVAL;
	}
      if (run_stop == CYCLOPS_STOP)
	adcm1700WindowSizeSetInputPan (capture.offset);	// perform input pan
      else
	{
	  if (capture.runTime >= 10)	// if the run time is too short, the event can be missed
	    {
	      ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID,
			      TIMER_ONE_SHOT);
	      ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID,
			       capture.runTime);
	    }
	  else
	    set_CPLD_run_mode (CPLD_OPCODE_CAPTURE_IMAGE, sPage, nFrames);
	}
      break;
    }

    // window pan position
  case SET_INPUT_PAN_DONE:
    {
      MsgParam *p = (MsgParam *) (msg->data);
      uint8_t status = p->byte;
      if (status != SOS_OK)
	{
	  pImg->result = -EINVAL;
	  post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		     sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	  //pImg = NULL;
	  controlState = CAMERA_READY;
	  return -EINVAL;
	}
      adcm1700CtrlRunSensor (CYCLOPS_RUN);
      break;
    }

  case GET_SUMS_DONE:
    {
      color16_result_t *m = (color16_result_t *) (msg->data);
      color16_t values = m->values;
      uint8_t status = m->result;
      getSumsDone (values, status);
      break;
    }

    /*
      case READ_REGISTER_DONE:
      break;

      case WRITE_REGISTER_DONE:
      break;

      case WRITE_BLOCK_DONE:
      break;

      case READ_BLOCK_DONE:
      break;
    */

  default:
    return -EINVAL;
  }
  return SOS_OK;
}

static int8_t
adcm1700_control_output_size_handler (Message * msg)
{
  MsgParam *p = (MsgParam *) (msg->data);
  uint8_t status = p->byte;
  if (status != SOS_OK)
    {
      switch (currentCommand)
	{
	case CINIT:
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			0, 0);
	  return -EINVAL;
	  break;
	case CSNAP:
	  pImg->result = -EINVAL;
	  post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		     sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	  //pImg = NULL;
	  controlState = CAMERA_READY;
	  return -EINVAL;
	  break;
	default:
	  break;
	}
    }
  switch (currentCommand)
    {
    case CINIT:
      adcm1700CtrlRunCamera (CYCLOPS_STOP);	// enter low-power standby mode
      break;
    case CSNAP:
      adcm1700CtrlRunCamera (CYCLOPS_RUN);	// start module running for capture
      break;
    default:
      break;
    }
  return SOS_OK;
}


static int8_t
adcm1700_control_camera_handler (Message * msg)
{
  MsgParam *p = (MsgParam *) (msg->data);
  uint8_t status = p->byte;
  //p->word is 16 bit, should i typecast?
  uint16_t run_stop = p->word;
  if (status != SOS_OK)
    {
      switch (currentCommand)
	{
	case CINIT:
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			0, 0);
	  return -EINVAL;
	  break;
	case CSNAP:
	  pImg->result = -EINVAL;
	  post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		     sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	  //pImg = NULL;
	  controlState = CAMERA_READY;
	  return -EINVAL;
	  break;
	case CRUN:
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EINVAL,
			0, 0);
	  return -EINVAL;
	default:
	  break;
	}
    }
  switch (currentCommand)
    {
    case CINIT:
      //TURN_CAMERA_CLOCK_OFF();        // end of init operation
      //controlState = CAMERA_READY;	// Camera is not completely READY as yet
      controlState = CAMERA_WARMING;
      //      signal imager.imagerReady(SOS_OK);
      // Ram - We can call the run method right here
      //      post_short(client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, SOS_OK, 0, 0);
      LED_DBG (LED_GREEN_ON);
      adcm1700control_run (5000);	// run the imager for 5 seconds (for AE convergence)  
      break;
    case CSNAP:
      if (run_stop == CYCLOPS_RUN)	// beginning of pan operation
	adcm1700CtrlRunSensor (CYCLOPS_STOP);
      else
	{
	  // end of capture operation: CPLD is already in SRAM access mode
	  pImg->result = SOS_OK;
	  if (client_pid != NULL_PID) {
	    post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		       sizeof (CYCLOPS_Image), pImg, SOS_MSG_RELEASE);
	  } else {
	    ker_free_handle(pImg->imageDataHandle);
	    ker_free(pImg);
	  }
	  //pImg = NULL;
	  controlState = CAMERA_READY;
	}
      break;
    case CRUN:
      if (run_stop == CYCLOPS_RUN)
	{
	  ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID, TIMER_ONE_SHOT);
	  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, runTime);
	}
      else
	{
	  // if we learn how to stop the clock, we will do something else here
	  //    signal imager.runDone(SOS_OK);
	  //      post_short(client_pid, ADCM1700_CONTROL_PID, RUN_DONE, SOS_OK, 0, 0);
	  controlState = CAMERA_READY;
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, SOS_OK, 0, 0);
	}
      break;
    default:
      break;
    }
  return SOS_OK;
}


static int8_t
adcm1700_control_timer_handler ()
{
  switch (currentCommand)
    {
      //-------------------------
      // INITIALIZE COMMAND
      //-------------------------
    case CINIT:
      switch (controlState)
	{

	case CAMERA_NOT_POWERED:
	  // DC power should be provided for > 20 mS before MCLK is activated
	  controlState = CAMERA_POWERED;
	  TURN_CAMERA_POWER_ON ();
	  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 100);
	  break;

	case CAMERA_POWERED:
	  // MCLK should be provided > 200 mS before the first I2C transaction
	  controlState = CAMERA_CLOCKED;
	  TURN_CAMERA_CLOCK_ON ();
	  ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 400);
	  // Clock should run for 200 mS before I2C communication
	  break;

	case CAMERA_CLOCKED:
	  // end of MCLK on delay
	  adcm1700CtrlPatchSetPatch ();
	  break;
								
	case CAMERA_PATCHED:
	case CAMERA_WARMING:		
	case CAMERA_BUSY:
	  // Camera is busy from previous client, or not initialized yet
	  // return -EBUSY to tell current client to wait
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, -EBUSY, 0, 0);
	  break;
										
	case CAMERA_READY:
	  // Camera is already initialized, just send SOS_OK
	  if (client_pid != NULL_PID)
	    post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, SOS_OK, 0, 0);
	  break;

	default:
	  break;
	}
      break;
      //-------------------------
      // PERFORM CAPTURE COMMAND
      //-------------------------                         
    case CSNAP:
      LED_DBG (LED_AMBER_TOGGLE);
      set_CPLD_run_mode (CPLD_OPCODE_CAPTURE_IMAGE, sPage, nFrames);
      break;
      //-------------------------
      // CAMERA RUN COMMAND
      //-------------------------
    case CRUN:
      adcm1700CtrlRunCamera (CYCLOPS_STOP);
      break;
    default:
      break;
    }
  return SOS_OK;
}

static int8_t
getSumsDone (color16_t values, uint8_t status)	// Ram - Status needs to change to int8_t
{
  color16_t averages;
  color16_result_t averageResult;
  uint16_t scaleFactor;

  if (status != SOS_OK)
    {
      averages.red = 0;
      averages.green = 0;
      averages.blue = 0;
      averageResult.values.red = averages.red;
      averageResult.values.green = averages.green;
      averageResult.values.blue = averages.blue;
      averageResult.result = -EINVAL;
      post_long (client_pid, ADCM1700_STAT_PID, GET_PIXEL_AVERAGES_DONE,
		 sizeof (averageResult), &averageResult, 0);
    }
  else
    {
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
      post_long (client_pid, ADCM1700_STAT_PID, GET_PIXEL_AVERAGES_DONE,
		 sizeof (averageResult), &averageResult, 0);
    }
  return SOS_OK;
}


// last argument used to be stopPage
static void set_CPLD_run_mode (uint8_t opcode, uint8_t startPage, uint8_t count)
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
  capture.exposurePeriod = 0x0000;	// AE
  capture.analogGain.red = 0x00;
  capture.analogGain.green = 0x00;
  capture.analogGain.blue = 0x00;
  capture.digitalGain.red = 0x0000;	// AWB
  capture.digitalGain.green = 0x0000;
  capture.digitalGain.blue = 0x0000;
  capture.runTime = 0;

  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

//----------------------------------------------------------------------------------------------------------
// KERNEL API
//----------------------------------------------------------------------------------------------------------
// Register a client with the imager
int8_t ker_registerImagerClient (sos_pid_t pid)
{
  if (NULL_PID != client_pid)
    {
      return -EBUSY;
    }
		
  if ( (controlState == CAMERA_READY) || (controlState == CAMERA_NOT_POWERED) ) {
    client_pid = pid;
    currentCommand = CINIT;
    ker_timer_init (ADCM1700_CONTROL_PID, CONTROL_TID, TIMER_ONE_SHOT);
    ker_timer_start (ADCM1700_CONTROL_PID, CONTROL_TID, 2000);
    return SOS_OK;
  } else {
    return -EBUSY;
  }
		
  //return SOS_OK;
}

//----------------------------------------------------------------------------------------------------------
// Release a client with the imager
int8_t ker_releaseImagerClient (sos_pid_t pid)
{
  if ((NULL_PID != pid) &&
      (pid == client_pid)){
    client_pid = NULL_PID;
    return SOS_OK;
  }
  return -EINVAL;
}

// capture an image and store it in SRAM             
int8_t ker_snapImage (CYCLOPS_ImagePtr ptrImg)
{
  uint8_t* imageDataPtr;
  uint16_t imgSize;
  currentCommand = CSNAP;
  LED_DBG (LED_YELLOW_TOGGLE);
  if (controlState != CAMERA_READY)	// in case the init routine -EINVALed ...
    {
      imageResult.ImgPtr = NULL;
      imageResult.result = -EINVAL;
      post_long (client_pid, ADCM1700_STAT_PID, SNAP_IMAGE_DONE,
		 sizeof (imageResult), &imageResult, 0);
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
    //pImg = NULL;
    return -EINVAL;
  }
		
  imageDataPtr = (uint8_t*) ker_get_handle_ptr(pImg->imageDataHandle);
  if (NULL == imageDataPtr) {
    return -EINVAL;
  }
  sPage = (uint8_t) (((uint16_t) (imageDataPtr)) >> 8);	//divide by 256 to get the page number
  nFrames = (pImg->nFrames & 0x0F);	// limit capture to 15 frames
  adcm1700CtrlFormatSetFormat (pImg->type);	// jump into the rabbit hole!
  controlState = CAMERA_BUSY;
  return SOS_OK;
}

// set new capture parameters
// (These values are not communicated to the module until the next capture is performed)
int8_t ker_setCaptureParameters (CYCLOPS_CapturePtr pCap)
{
  currentCommand = CSET;
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

  // Ram - We do not need to make this split phase 
  //    signal imager.setCaptureParametersDone(SOS_OK);
  //  post_short(client_pid, ADCM1700_CONTROL_PID, SET_CAPTURE_PARAMETERS_DONE, SOS_OK, 0, 0);
  return SOS_OK;
}

// Get the average pixel values
uint8_t
getPixelAverages ()
{
  currentCommand = CSTAT;

  adcm1700CtrlStatGetSums ();
  return SOS_OK;
}

// run the imager for the specified period
static int8_t adcm1700control_run (uint16_t rTime)
{
  currentCommand = CRUN;
  runTime = rTime;

  if (runTime >= 10){
    TURN_CAMERA_CLOCK_ON ();	// activate MCLK
    adcm1700CtrlRunCamera (CYCLOPS_RUN);
  }
  else{
    //    post_short(client_pid, ADCM1700_CONTROL_PID, RUN_DONE, SOS_OK, 0, 0);
    if (client_pid != NULL_PID)
      post_short (client_pid, ADCM1700_CONTROL_PID, IMAGER_READY, SOS_OK, 0, 0);
  }
  return SOS_OK;
}
