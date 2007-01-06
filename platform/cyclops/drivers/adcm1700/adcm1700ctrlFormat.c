// Authors : 
//           Mohammad Rahimi mhr@cens.ucla.edu 
//           Rick Baer       rick_baer@agilent.com 
// Ported by:
//           Kevin Lee       kevin79@ucla.edu
// This component sets the image type (e.g. monochrome, color, etc.) 

//#include <sos_module_types.h>
#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlFormat.h>
#include <memConst.h>
#include <image.h>

static int8_t adcm1700_format_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id:ADCM1700_FORMAT_PID,
state_size:0,
num_timers:0,
num_sub_func:0,
num_prov_func:0,
module_handler:adcm1700_format_handler,
};

// current state of component (and camera module)
static uint8_t currentFormat;

//*******************Inititialization and Termination*********************/
int8_t
adcm1700CtrlFormatInit ()
{
	currentFormat = CYCLOPS_IMAGE_TYPE_UNSET;	// force initialization
	ker_register_module (sos_get_header_address (mod_header));
	return SOS_OK;
}

static int8_t
adcm1700_format_handler (void *state, Message * msg)
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
		case WRITE_REGISTER_DONE:
		{
			register_result_t *m = (register_result_t *) (msg->data);
			//      uint16_t addr = m->reg;
			//      uint16_t data = m->data;
			uint8_t status = m->status;
			
			if (status != SOS_OK)
			{
				post_short (ADCM1700_CONTROL_PID, ADCM1700_FORMAT_PID,
							SET_FORMAT_DONE, -EINVAL, 0, 0);
				return -EINVAL;
			}
			post_short (ADCM1700_CONTROL_PID, ADCM1700_FORMAT_PID,
						SET_FORMAT_DONE, SOS_OK, 0, 0);
			return SOS_OK;
		}
			
		case READ_REGISTER_DONE:
		{
			register_result_t *m = (register_result_t *) (msg->data);
			//      uint16_t addr = m->reg;
			uint16_t data = m->data;
			uint8_t status = m->status;
			
			if (status != SOS_OK)	// need current register value to proceed
			{
				post_short (ADCM1700_CONTROL_PID, ADCM1700_FORMAT_PID,
							SET_FORMAT_DONE, -EINVAL, 0, 0);
				return -EINVAL;
			}
			switch (currentFormat)	// set the new video format
			{
				case CYCLOPS_IMAGE_TYPE_RGB:
					writeRegister (ADCM1700_FORMAT_PID, ADCM_REG_OUTPUT_CTRL_V,
								   ((data & 0xFFF0) | ADCM_OUTPUT_FORMAT_RGB));
					break;
				case CYCLOPS_IMAGE_TYPE_YCbCr:
					writeRegister (ADCM1700_FORMAT_PID, ADCM_REG_OUTPUT_CTRL_V,
								   ((data & 0xFFF0) | ADCM_OUTPUT_FORMAT_YCbCr));
					break;
				case CYCLOPS_IMAGE_TYPE_RAW:
					writeRegister (ADCM1700_FORMAT_PID, ADCM_REG_OUTPUT_CTRL_V,
								   ((data & 0xFFF0) | ADCM_OUTPUT_FORMAT_RAW));
					break;
				case CYCLOPS_IMAGE_TYPE_Y:
					writeRegister (ADCM1700_FORMAT_PID, ADCM_REG_OUTPUT_CTRL_V,
								   ((data & 0xFFF0) | ADCM_OUTPUT_FORMAT_Y));
					break;
				default:
					post_short (ADCM1700_CONTROL_PID, ADCM1700_FORMAT_PID,
								SET_FORMAT_DONE, -EINVAL, 0, 0);
					return -EINVAL;
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
		case SET_FORMAT_DONE:
		{
			return SOS_OK;
		}
		default:
			return -EINVAL;
    }
	return SOS_OK;
}

//***********************Format Control*********************/
int8_t
adcm1700CtrlFormatSetFormat (uint8_t myType)
{
  // force update
  if (myType != currentFormat)
    {
      currentFormat = myType;	// save format for subsequent use
      readRegister (ADCM1700_FORMAT_PID, ADCM_REG_OUTPUT_CTRL_V);	// everything else happens in the call backs
    }
  else
    post_short (ADCM1700_CONTROL_PID, ADCM1700_FORMAT_PID, SET_FORMAT_DONE,
		SOS_OK, 0, 0);
  return SOS_OK;
}
