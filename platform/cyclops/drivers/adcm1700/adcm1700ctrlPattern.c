// Authors : 
//           Mohammad Rahimi mhr@cens.ucla.edu 
//           Rick Baer       rick_baer@agilent.com 
// Ported By:
//           Kevin Lee       kevin79@ucla.edu
// This component selects between normal imaging and the test pattern 
// generator. Test patterns are injected at the input to the image processing
// pipeline.
// The DATA_GEN register (0x105e) is used to select test modes. Valid values
// include:
//    0x00   normal imaging mode
//    0x03   sum of coordinates pattern
//    0x04   black field with 8-pixel wide white border
//    0x05   checkerboard test
//    0x07   color bars
// The upper 8-bits of the DATA_GEN register are reserved.

#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlPattern.h>
#include <memConst.h>
#include <image.h>

static int8_t adcm1700_pattern_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id:ADCM1700_PATTERN_PID,
state_size:0,
num_timers:0,
num_sub_func:0,
num_prov_func:0,
module_handler:adcm1700_pattern_handler,
};

// 
static uint8_t currentPattern;	// current state of component (and module)

//*******************Inititialization and Termination*********************/
int8_t
adcm1700CtrlPatternInit ()
{
  currentPattern = CYCLOPS_TEST_MODE_UNSET;	// default is normal imaging
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_pattern_handler (void *state, Message * msg)
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
    case SET_PATTERN_DONE:
      {
	return SOS_OK;
      }

      //***********************Writing and Reading individual Bytes*********************/
    case WRITE_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	//uint16_t addr = m->reg;
	//uint16_t data = m->data;
	uint8_t result = m->status;
	post_short (ADCM1700_CONTROL_PID, ADCM1700_PATTERN_PID,
		    SET_PATTERN_DONE, result, 0, 0);
	return SOS_OK;
      }

    case READ_REGISTER_DONE:
      {
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


//***********************Format Control*********************/
int8_t
adcm1700CtrlSetPattern (uint8_t myPattern)
{
  // perform value checking here!
  if (myPattern != currentPattern)
    {
      currentPattern = myPattern;
      switch (myPattern)
	{
	case CYCLOPS_TEST_MODE_NONE:
	  writeRegister (ADCM1700_PATTERN_PID, ADCM_REG_DATA_GEN,
			 ADCM_TEST_MODE_NONE);
	  break;
	case CYCLOPS_TEST_MODE_SOC:
	  writeRegister (ADCM1700_PATTERN_PID, ADCM_REG_DATA_GEN,
			 ADCM_TEST_MODE_SOC);
	  break;
	case CYCLOPS_TEST_MODE_8PB:
	  writeRegister (ADCM1700_PATTERN_PID, ADCM_REG_DATA_GEN,
			 ADCM_TEST_MODE_8PB);
	  break;
	case CYCLOPS_TEST_MODE_CKB:
	  writeRegister (ADCM1700_PATTERN_PID, ADCM_REG_DATA_GEN,
			 ADCM_TEST_MODE_CKB);
	  break;
	case CYCLOPS_TEST_MODE_BAR:
	  writeRegister (ADCM1700_PATTERN_PID, ADCM_REG_DATA_GEN,
			 ADCM_TEST_MODE_BAR);
	  break;
	default:
	  post_short (ADCM1700_CONTROL_PID, ADCM1700_PATTERN_PID,
		      SET_PATTERN_DONE, -EINVAL, 0, 0);
	  break;
	}
    }
  else
    post_short (ADCM1700_CONTROL_PID, ADCM1700_PATTERN_PID, SET_PATTERN_DONE,
		SOS_OK, 0, 0);
  return SOS_OK;
}
