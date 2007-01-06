// Authors : 
//           Mohammad Rahimi mhr@cens.ucla.edu 
//           Rick Baer       rick_baer@agilent.com 
// Ported By: 
//           Kevin Lee       kevin79@ucla.edu
//includes imagerConst;

#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlPatch.h>
#include <memConst.h>
#include <image.h>

#define PATCH_TID               1

static int8_t adcm1700_patch_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
 mod_id:ADCM1700_PATCH_PID,
 state_size:0,
 num_timers:1,
 num_sub_func:0,
 num_prov_func:0,
 module_handler:adcm1700_patch_handler,
};

static uint16_t controlState;
/* patch code, statically initialized */
static uint16_t patchNumber = 0;	/* index into the arrays below */
//static uint16_t patch1700addr[]= {0x0186, 0x4808, 0x4820, 0x4860, 0x486C, 0x4880, 0};
static uint16_t patch1700len[] = { 4, 4, 4, 4, 4, (2 * (8 * 12 + 2)), 0 };
static uint16_t patchdata_index = 0;	/* index into the array below */
static char patch1700data[] = {
  0x01, 0x04, 0x09, 0x00,
  0x00, 0x02, 0x42, 0x20,
  0x00, 0x02, 0x42, 0x28,
  0x00, 0x02, 0x42, 0x24,
  0x00, 0x02, 0x42, 0x2E,
  0x00, 0x0F, 0x01, 0x60, 0x00, 0x00, 0x7E, 0x80, 0x00, 0x00, 0x7E, 0x90,
  0x00, 0x02, 0x04, 0x2F,
  0x00, 0x00, 0x8E, 0xA2, 0x00, 0x05, 0x42, 0x27, 0x00, 0x03, 0x0B, 0x50,
  0x00, 0x00, 0x01, 0xFF,
  0x00, 0x00, 0x06, 0x48, 0x00, 0x05, 0x42, 0x2C, 0x00, 0x00, 0x9A, 0xA1,
  0x00, 0x05, 0x08, 0x00,
  0x00, 0x00, 0x06, 0x28, 0x00, 0x02, 0x07, 0xBD, 0x00, 0x00, 0x5D, 0x3F,
  0x00, 0x00, 0x59, 0x40,
  0x00, 0x00, 0x04, 0x16, 0x00, 0x00, 0x02, 0x77, 0x00, 0x00, 0x01, 0xA7,
  0x00, 0x00, 0x04, 0x3F,
  0x00, 0x00, 0x06, 0x4E, 0x00, 0x04, 0x42, 0x37, 0x00, 0x00, 0x04, 0x27,
  0x00, 0x00, 0x5B, 0x4C,
  0x00, 0x00, 0x01, 0x7E, 0x00, 0x07, 0x42, 0x42, 0x00, 0x00, 0x5B, 0x4D,
  0x00, 0x00, 0x01, 0x7E,
  0x00, 0x07, 0x42, 0x42, 0x00, 0x00, 0x5B, 0x4E, 0x00, 0x00, 0x01, 0x7E,
  0x00, 0x07, 0x42, 0x42,
  0x00, 0x00, 0x06, 0x0E, 0x00, 0x02, 0x09, 0x34, 0x00, 0x00, 0x06, 0x2E,
  0x00, 0x0F, 0x00, 0x18,
  0x00, 0x00, 0x5B, 0x4D, 0x00, 0x00, 0x01, 0x7E, 0x00, 0x07, 0x42, 0x4A,
  0x00, 0x00, 0x5B, 0x4E,
  0x00, 0x00, 0x01, 0x7E, 0x00, 0x06, 0x42, 0x4B, 0x00, 0x00, 0x01, 0xFF,
  0x00, 0x00, 0x5E, 0xBF,
  0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0xF7, 0x00, 0x00, 0x04, 0x07,
  0x00, 0x00, 0x00, 0x77,
  0x00, 0x02, 0x09, 0x3A
};

enum
  {
    CONTROL_STATE_PENDING_CONFIGURATION = 0,
    CONTROL_STATE_SETTING_DEVICE,
    CONTROL_STATE_END,		/* wait for config */
    CONTROL_STATE_START_CFG,	/* bring start up configuration online */
    CONTROL_STATE_READY,		/* ready for run-time commands */
  };

//*******************Inititialization and Termination*********************/

int8_t
adcm1700CtrlPatchInit ()
{
  controlState = CONTROL_STATE_PENDING_CONFIGURATION;
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_patch_handler (void *state, Message * msg)
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
	patchNumber = 0;
	patchdata_index = 0;
	return SOS_OK;
      }
    case SET_PATCH_DONE:
      {
	return SOS_OK;
      }
			
      //******************* Timer (allows enough time for module to exit run mode) ******
    case MSG_TIMER_TIMEOUT:
      {
	writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR1,
		    &patch1700data[patchdata_index],
		    (uint8_t) patch1700len[patchNumber]);
	return SOS_OK;
      }
			
      //***********************Writing and Reading individual Bytes*********************/
    case READ_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	uint16_t data = m->data;
	uint8_t result = m->status;
			
	//mhr:note that it is ok if the status fails when the device is stll configuring itself since we can not communicate with it in the meantime, we retry
	if ((controlState != CONTROL_STATE_PENDING_CONFIGURATION)
	    && (result != SOS_OK))
	  {
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
			SET_PATCH_DONE, -EINVAL, 0, 0);
	    return -EINVAL;
	  }
	switch (addr)
	  {
	    HAS_CRITICAL_SECTION;
	  case ADCM_REG_STATUS:	//mhr:checking if the device is configured and now ready to be set.
	    if (controlState == CONTROL_STATE_PENDING_CONFIGURATION)
	      {			// part of startup
		if (result == SOS_OK)
		  {
		    if (data & ADCM_STATUS_CONFIG_MASK)	// still configuring, check again.  Should probably add a delay
		      readRegister (ADCM1700_PATCH_PID, ADCM_REG_STATUS);
		    else	// it is now configured, we officialy go the mode to set different registers of the device.
		      {
			ENTER_CRITICAL_SECTION ();
			{
			  controlState = CONTROL_STATE_SETTING_DEVICE;
			}
			LEAVE_CRITICAL_SECTION ();
			//atomic controlState = CONTROL_STATE_SETTING_DEVICE;
			readRegister (ADCM1700_PATCH_PID, ADCM_REG_ID);
		      }
		  }
		else
		  readRegister (ADCM1700_PATCH_PID, ADCM_REG_STATUS);
	      }
	    break;
	  case ADCM_REG_ID:	//checking if we have the right imager
	    if (controlState == CONTROL_STATE_SETTING_DEVICE)
	      {
		// continue with startup intialization, stop the imager to re-configure it. CONTROL register is read first and the RUN bit is turned off
		if ((ADCM_ID_MASK & data) == (ADCM_ID_MASK & ADCM_ID_1700))
		  readRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL);
		// had a failure (incorrect module ID)
		else
		  {
		    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
				SET_PATCH_DONE, -EINVAL, 0, 0);
		    return -EINVAL;
		  }
	      }
	    break;
	  case ADCM_REG_CONTROL:
	    switch (controlState)
	      {
	      case CONTROL_STATE_SETTING_DEVICE:	// clear the RUN bit and write it out
		writeRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL,
			       (data & ~ADCM_CONTROL_RUN_MASK));
		break;
	      case CONTROL_STATE_END:	// Bring the new configuration online by setting the CONFIG bit in the CONTROL register                         
		ENTER_CRITICAL_SECTION ();
		{
		  controlState = CONTROL_STATE_START_CFG;
		}
		LEAVE_CRITICAL_SECTION ();
		//atomic controlState = CONTROL_STATE_START_CFG;
		// was ADCM_CONTROL_CONFIG_MASK:   RLB 3/18/05
		writeRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL,
			       (data | ADCM_CONTROL_RUN_MASK));
		break;
	      case CONTROL_STATE_START_CFG:	// initializing the imager, and now bringing the configuration online. check if still configuring. 
		if ((data & ADCM_CONTROL_CONFIG_MASK) == 0)	// CONFIG bit has cleared, we are done 
		  {
		    ENTER_CRITICAL_SECTION ();
		    {
		      controlState = CONTROL_STATE_READY;
		    }
		    LEAVE_CRITICAL_SECTION ();
		    //atomic controlState = CONTROL_STATE_READY;
		    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
				SET_PATCH_DONE, SOS_OK, 0, 0);
		  }
		else		// re-read and check bit again                                
		  readRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL);
	      default:
		break;
	      }
	    break;
	  case ADCM_REG_OUTPUT_CTRL_V:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_OUTPUT_CTRL_V,
			   (data | ADCM_OUTPUT_CTRL_FRVCLK_MASK |
			    ADCM_OUTPUT_CTRL_FR_OUT_MASK |
			    ADCM_OUTPUT_CTRL_JUST_MASK |
			    ADCM_OUTPUT_FORMAT_Y));
	    break;
	  case ADCM_REG_PROC_CTRL_V:
	    if (controlState == CONTROL_STATE_SETTING_DEVICE)
	      writeRegister (ADCM1700_PATCH_PID, ADCM_REG_PROC_CTRL_V,
			     (data & ~ADCM_PROC_CTRL_NOSIZER));
	    break;
	  default:
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
			SET_PATCH_DONE, -EINVAL, 0, 0);
	    return -EINVAL;
	  }
	return SOS_OK;
      }
			
    case WRITE_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	//uint16_t data = m->data;
	uint8_t result = m->status;
			
	if (result != SOS_OK)
	  {
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
			SET_PATCH_DONE, -EINVAL, 0, 0);
	    return -EINVAL;
	  }
	switch (addr)
	  {
	    HAS_CRITICAL_SECTION;
	  case ADCM_REG_CONTROL:
	    switch (controlState)
	      {
	      case CONTROL_STATE_SETTING_DEVICE:	// start patching code                                             
		ker_timer_init (ADCM1700_PATCH_PID, PATCH_TID,
				TIMER_ONE_SHOT);
		ker_timer_start (ADCM1700_PATCH_PID, PATCH_TID, 500);
		break;
	      case CONTROL_STATE_END:	// read the register to check for the config bit 
		readRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL);
		break;
	      case CONTROL_STATE_START_CFG:
		// just set CONFIG bit, read the register to see if it has been cleared, indicating it is done. callback for this read will check the CONFIG bit
		readRegister (ADCM1700_PATCH_PID, ADCM_REG_CONTROL);
		break;
	      default:
		break;
	      }
	    break;
	  case ADCM_REG_OUTPUT_CTRL_V:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_CLK_PER, 0x0FA0);	// 4 MHz (4,000 kHz)
	    break;
	  case ADCM_REG_CLK_PER:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_AE2_ETIME_MIN, 0x0001);	// 10 microseconds
	    break;
	  case ADCM_REG_AE2_ETIME_MIN:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_AE2_ETIME_MAX, 0xC350);	// 0.5 seconds
	    break;
	  case ADCM_REG_AE2_ETIME_MAX:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_AE_GAIN_MAX, 0x0F00);	// analog gain = 15
	    break;
	  case ADCM_REG_AE_GAIN_MAX:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_AF_CTRL1, 0x0013);	// auto functions activated
	    break;
	  case ADCM_REG_AF_CTRL1:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_AF_CTRL2, 0x0002);	// no overexposure, 60 Hz
	    break;
	  case ADCM_REG_AF_CTRL2:
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_SZR_IN_WID_V,
			   ADCM_SIZE_1700DEFAULT_W);
	    break;
	  case ADCM_REG_SZR_IN_WID_V:	//step sizer:1:9
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_SZR_IN_HGT_V,
			   ADCM_SIZE_1700DEFAULT_H);
	    break;
	  case ADCM_REG_SZR_IN_HGT_V:	//step sizer:2:9
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_SZR_OUT_WID_V,
			   ADCM_SIZE_API_DEFAULT_W);
	    break;
	  case ADCM_REG_SZR_OUT_WID_V:	//step sizer:5:9
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_SZR_OUT_HGT_V,
			   ADCM_SIZE_API_DEFAULT_H);
	    break;
	  case ADCM_REG_SZR_OUT_HGT_V:	//step sizer:6:9
	    readRegister (ADCM1700_PATCH_PID, ADCM_REG_PROC_CTRL_V);
	    break;
	  case ADCM_REG_PROC_CTRL_V:	// set window size: step 8 of 9
	    writeRegister (ADCM1700_PATCH_PID, ADCM_REG_VBLANK_V,
			   ADCM_VBLANK_DEFAULT);
	    break;
	  case ADCM_REG_VBLANK_V:	// set vblank interval: step 9 of 9
					
	    ENTER_CRITICAL_SECTION ();
	    {
	      controlState = CONTROL_STATE_READY;
	    }
	    LEAVE_CRITICAL_SECTION ();
	    //atomic controlState = CONTROL_STATE_READY;
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
			SET_PATCH_DONE, SOS_OK, 0, 0);
	    return SOS_OK;
					
	    break;
	  }
	return SOS_OK;
      }
			
      //***********************Writing and Reading Blocks*********************/
    case WRITE_BLOCK_DONE:
      {
	block_result_t *m = (block_result_t *) (msg->data);
	uint16_t startReg = m->startReg;
	//char *data = m->data;
	//uint8_t length = m->length;
	uint8_t result = m->result;
			
	if (result != SOS_OK)
	  post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
		      SET_PATCH_DONE, -EINVAL, 0, 0);
	switch (startReg)
	  {
	  case ADCM_REG_PADR1:	// write the next patch block 
	    patchdata_index = patchdata_index + patch1700len[patchNumber];
	    ++patchNumber;
	    writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR2,
			&patch1700data[patchdata_index],
			(uint8_t) patch1700len[patchNumber]);
	    break;
	  case ADCM_REG_PADR2:	// write the next patch block 
	    patchdata_index = patchdata_index + patch1700len[patchNumber];
	    ++patchNumber;
	    writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR3,
			&patch1700data[patchdata_index],
			(uint8_t) patch1700len[patchNumber]);
	    break;
	  case ADCM_REG_PADR3:	// write the next patch block 
	    patchdata_index = patchdata_index + patch1700len[patchNumber];
	    ++patchNumber;
	    writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR4,
			&patch1700data[patchdata_index],
			(uint8_t) patch1700len[patchNumber]);
	    break;
	  case ADCM_REG_PADR4:	// write the next patch block 
	    patchdata_index = patchdata_index + patch1700len[patchNumber];
	    ++patchNumber;
	    writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR5,
			&patch1700data[patchdata_index],
			(uint8_t) patch1700len[patchNumber]);
	    break;
	  case ADCM_REG_PADR5:	// write the next patch block 
	    patchdata_index = patchdata_index + patch1700len[patchNumber];
	    ++patchNumber;
	    writeBlock (ADCM1700_PATCH_PID, ADCM_REG_PADR6, &patch1700data[patchdata_index], (uint8_t) patch1700len[patchNumber]);	// *** diagnostic ***
	    break;
	  case ADCM_REG_PADR6:	// Patching dine, set data protocol that the CPLD expects,first read the register value and set the new data in the callback
	    readRegister (ADCM1700_PATCH_PID, ADCM_REG_OUTPUT_CTRL_V);
	    break;
	  default:
	    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATCH_PID,
			SET_PATCH_DONE, -EINVAL, 0, 0);
	    break;
	  }
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

//***********************Patch Control*********************/
int8_t
adcm1700CtrlPatchSetPatch ()
{
  //mhr: This component is reentrant. There is no provision to avoid calling it while it is still running
  patchNumber = 0;
  patchdata_index = 0;
  controlState = CONTROL_STATE_PENDING_CONFIGURATION;
  readRegister (ADCM1700_PATCH_PID, ADCM_REG_STATUS);	// Most of the initialization is performed in the callback routine
  return SOS_OK;
}
