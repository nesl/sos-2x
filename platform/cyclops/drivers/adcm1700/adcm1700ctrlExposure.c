// Authors : 
//           Rick Baer       rick_baer@agilent.com 
//           Mohammad Rahimi mhr@cens.ucla.edu 
// Ported by:
//           Kevin Lee       kevin79@ucla.edu
//#include <sos_module_types.h>
#include <module.h>
#include <math.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlExposure.h>
#include <memConst.h>
#include <image.h>

#define MCLK 4.0e6		// sensor clock frequency

static int8_t adcm1700_exposure_handler (void *state, Message * e);
//static sos_module_t adcm1700_exposure_module;
static mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id:ADCM1700_EXPOSURE_PID,
state_size:0,
num_timers:0,
num_sub_func:0,
num_prov_func:0,
module_handler:adcm1700_exposure_handler,
};

//********* current state variables *******
static float currentExposureTime;	// Time in seconds [floating point variable]
static color8_t currentAnalogGain;	// ADCM PGA gains are 8 bits: (1 + b[6]) * (1 + 0.15 * b[5:0]/(1 + 2 * b[7]))
static color16_t currentDigitalGain;	// ADCM digital gains are 10 bits (binary 3.7 format, or gain * 128)

// This data is used in manual exposure calculations
static uint8_t cpp;		// sensor clocks per pixel
static uint8_t hblank;		// horizontal blanking interval (in column processing periods)
static uint8_t vblank;		// vertical blanking interval (in row processing periods)
static uint16_t rows;		// number of image rows
static uint16_t cols;		// number of columns
static uint16_t rowexp;		// exposure period (in row processing periods)
static uint8_t subrow;		// subrow exposure
static uint8_t subMAX;		// subrow exposure limit
static uint8_t currentCommand;

enum
{
  EXTIME,
  EAGAIM,
  EDGAIN
};
//*******************Inititialization and Termination*********************/
int8_t
adcm1700CtrlExposureInit ()
{
  // establish default values
  currentExposureTime = 0.0;
  currentAnalogGain.red = 0;	// auto exposure
  currentAnalogGain.green = 0;
  currentAnalogGain.blue = 0;
  currentDigitalGain.red = 0;	// auto white balance
  currentDigitalGain.green = 0;
  currentDigitalGain.blue = 0;

  rowexp = 0x0070;		// default exposure values
  subrow = 0x00;
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_exposure_handler (void *state, Message * msg)
{
  // i2c_msg_state_t *s = (i2c_msg_state_t*)state;
  switch (msg->type)
    {
    case MSG_INIT:
      {
	return SOS_OK;
      }
    case MSG_FINAL:
      {
	return SOS_OK;
      }
    case SET_EXPOSURE_TIME_DONE:
      return SOS_OK;
    case SET_ANALOG_GAIN_DONE:
      return SOS_OK;
    case SET_DIGITAL_GAIN_DONE:
      return SOS_OK;

      //***********************Writing and Reading individual Bytes*********************/
      //event result_t imagerComm.writeRegisterDone (uint16_t addr, uint16_t data,result_t status)
      //Kevin: I need to struct this
    case WRITE_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
//                      uint16_t data = m->data;
	uint8_t status = m->status;

	if (status != SOS_OK)
	  {
	    switch (currentCommand)
	      {
	      case EXTIME:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_EXPOSURE_TIME_DONE, -EINVAL, 0, 0);
		break;
	      case EAGAIM:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_ANALOG_GAIN_DONE, -EINVAL, 0, 0);
		break;
	      case EDGAIN:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_DIGITAL_GAIN_DONE, -EINVAL, 0, 0);
		break;
	      }
	    return -EINVAL;
	  }

	switch (addr)
	  {
	    // auto versus manual control
	  case ADCM_REG_AF_CTRL1:
	    switch (currentCommand)
	      {
	      case EXTIME:
		if (currentExposureTime == 0)
		  post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			      SET_EXPOSURE_TIME_DONE, SOS_OK, 0, 0);
		else
		  readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_CPP_V);
		break;
	      case EAGAIM:
		if (currentAnalogGain.red == 0)
		  post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			      SET_ANALOG_GAIN_DONE, SOS_OK, 0, 0);
		else
		  writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_EREC_PGA,
				 currentAnalogGain.green);
		break;
	      case EDGAIN:
		if (currentDigitalGain.red == 0)
		  post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			      SET_DIGITAL_GAIN_DONE, SOS_OK, 0, 0);
		else
		  writeRegister (ADCM1700_EXPOSURE_PID,
				 ADCM_REG_APS_COEF_BLUE,
				 currentDigitalGain.blue);
		break;
	      }
	    break;

	    // set exposure
	    // *** These commands don't work if they are issued in the opposite order ??? ***
	  case ADCM_REG_ROWEXP_L:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_ROWEXP_H,
			   ((uint8_t) ((rowexp & 0x7F) >> 8)));
	    break;
	  case ADCM_REG_ROWEXP_H:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_SROWEXP, subrow);	// placeholder
	    break;
	  case ADCM_REG_SROWEXP:
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			SET_EXPOSURE_TIME_DONE, SOS_OK, 0, 0);
	    break;

	    // Analog gain
	  case ADCM_REG_EREC_PGA:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_EROC_PGA,
			   currentAnalogGain.red);
	    break;
	  case ADCM_REG_EROC_PGA:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_OREC_PGA,
			   currentAnalogGain.blue);
	    break;
	  case ADCM_REG_OREC_PGA:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_OROC_PGA,
			   currentAnalogGain.green);
	    break;
	  case ADCM_REG_OROC_PGA:
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			SET_ANALOG_GAIN_DONE, SOS_OK, 0, 0);
	    break;

	    // Digital gain
	  case ADCM_REG_APS_COEF_BLUE:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_APS_COEF_GRN1,
			   currentDigitalGain.green);
	    break;
	  case ADCM_REG_APS_COEF_GRN1:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_APS_COEF_GRN2,
			   currentDigitalGain.green);
	    break;
	  case ADCM_REG_APS_COEF_GRN2:
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_APS_COEF_RED,
			   currentDigitalGain.red);
	    break;
	  case ADCM_REG_APS_COEF_RED:
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			SET_DIGITAL_GAIN_DONE, SOS_OK, 0, 0);
	    break;

	  default:
	    break;
	  }

	return SOS_OK;
      }

    case READ_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	uint16_t data = m->data;
	uint8_t status = m->status;

	uint16_t nRS, nHB, nLA, nCP, nRPT, nSUB;
	float fRPT, fSUB;

	if (status != SOS_OK)
	  {
	    switch (currentCommand)
	      {
	      case EXTIME:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_EXPOSURE_TIME_DONE, -EINVAL, 0, 0);
		break;
	      case EAGAIM:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_ANALOG_GAIN_DONE, -EINVAL, 0, 0);
		break;
	      case EDGAIN:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
			    SET_DIGITAL_GAIN_DONE, -EINVAL, 0, 0);
		break;
	      }
	    return -EINVAL;
	  }

	switch (addr)
	  {
	  case ADCM_REG_AF_CTRL1:	// auto functions
	    switch (currentCommand)
	      {
	      case EXTIME:
	      case EAGAIM:
		if ((currentExposureTime == 0)
		    || (currentAnalogGain.red == 0))
		  writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1, (data | ADCM_AF_AE));	// activate AE
		else
		  writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1, (data & ~ADCM_AF_AE));	// de-activate AE
		break;
	      case EDGAIN:
		if (currentDigitalGain.red == 0)
		  writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1, (data | ADCM_AF_AWB));	// activate AWB
		else
		  writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1, (data & ~ADCM_AF_AWB));	// de-activate AWB
		break;
	      }
	    break;
	  case ADCM_REG_CPP_V:	// collect values for exposure time calculation
	    cpp = data;
	    readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_HBLANK_V);
	    break;
	  case ADCM_REG_HBLANK_V:
	    hblank = data;
	    readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_VBLANK_V);
	    break;
	  case ADCM_REG_VBLANK_V:
	    vblank = data;
	    readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_SENSOR_WID_V);
	    break;
	  case ADCM_REG_SENSOR_WID_V:
	    cols = data;
	    readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_SENSOR_HGT_V);
	    break;
	  case ADCM_REG_SENSOR_HGT_V:
	    rows = data;

	    // calculate exposure time, then program row exposure and subrow exposure
	    nRS = ceil (33 / cpp);	// reset period?
	    nHB = 2 * hblank * cpp;	// horizontal blanking
	    nLA = 10 * cpp;	// column latch time?
	    nCP = cols * cpp;	// column processing
	    nRPT = nRS + nHB + nLA + nCP;	// row processing time (sensor clocks)
	    fRPT = ((float) nRPT) / MCLK;	// row processing time (seconds)
	    rowexp = ((uint16_t) floor (currentExposureTime / fRPT));	// integer part of exposure (rows)
	    fSUB = currentExposureTime - rowexp * fRPT;	// fractional part of exposure
	    nSUB = ((uint16_t) (MCLK * fSUB));	// number of subrow clocks
	    subrow = (uint8_t) (1 + (((uint16_t) ((float) (nRPT - nRS - nSUB - 10.5)) / (4 * cpp)) & 0x00FF));	// needs work !!!
	    subMAX =
	      (uint8_t) (1 +
			 (((uint16_t)
			   floor (((float) (nRPT - nRS - 24)) /
				  (4 * cpp))) & 0x00FF));
	    subrow = subrow > subMAX ? subMAX : subrow;
	    writeRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_ROWEXP_L, ((uint8_t) (rowexp & 0xFF)));	// placeholder
	    break;
	  default:
	    break;
	  }
	return SOS_OK;
      }

      //***********************Writing and Reading Blocks*********************/
    case WRITE_BLOCK_DONE:
      {
	return SOS_OK;
      }
    case READ_BLOCK_DONE:
      {
	return SOS_OK;
      }
    default:
      return -EINVAL;
    }
  return SOS_OK;
}


//***********************Exposure Control*********************/
int8_t
adcm1700CtrlSetExposureTime (float exposureTime)
{
  currentCommand = EXTIME;
  // check for changes
  if (exposureTime != currentExposureTime)
    {
      currentExposureTime = exposureTime;
      readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1);
    }
  else
    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
		SET_EXPOSURE_TIME_DONE, SOS_OK, 0, 0);
  return SOS_OK;
}

int8_t
adcm1700CtrlSetAnalogGain (color8_t analogGain)
{
  currentCommand = EAGAIM;

  if ((analogGain.red != currentAnalogGain.red)
      | (analogGain.green != currentAnalogGain.green)
      | (analogGain.blue != currentAnalogGain.blue))
    {
      currentAnalogGain.red = analogGain.red;
      currentAnalogGain.green = analogGain.green;
      currentAnalogGain.blue = analogGain.blue;

      readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1);
    }
  else
    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
		SET_ANALOG_GAIN_DONE, SOS_OK, 0, 0);
  return SOS_OK;
}

int8_t
adcm1700CtrlSetDigitalGain (color16_t digitalGain)
{
  currentCommand = EDGAIN;

  if ((digitalGain.red != currentDigitalGain.red)
      | (digitalGain.green != currentDigitalGain.green)
      | (digitalGain.blue != currentDigitalGain.blue))
    {
      currentDigitalGain.red = digitalGain.red;
      currentDigitalGain.green = digitalGain.green;
      currentDigitalGain.blue = digitalGain.blue;
      readRegister (ADCM1700_EXPOSURE_PID, ADCM_REG_AF_CTRL1);
    }
  else
    post_short (ADCM1700_CONTROL_PID, ADCM1700_EXPOSURE_PID,
		SET_DIGITAL_GAIN_DONE, SOS_OK, 0, 0);
  return SOS_OK;
}
