// Authors : 
//           Mohammad Rahimi mhr@cens.ucla.edu 
//           Rick Baer       rick_baer@agilent.com 
// Ported By :
//           Kevin Lee       kevin79@ucla.edu

#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlStat.h>
#include <memConst.h>

static int8_t adcm1700_stat_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id:ADCM1700_STAT_PID,
state_size:0,
num_timers:0,
num_sub_func:0,
num_prov_func:0,
module_handler:adcm1700_stat_handler,
};

static color16_result_t colorResult;

//*******************Inititialization and Termination*********************/
int8_t
adcm1700CtrlStatInit ()
{
  colorResult.values.red = 0;
  colorResult.values.green = 0;
  colorResult.values.blue = 0;
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_stat_handler (void *state, Message * msg)
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
    case GET_SUMS_DONE:
      {
	return SOS_OK;
      }

      //***********************Writing and Reading individual Bytes*********************/
    case WRITE_REGISTER_DONE:
      {
	return SOS_OK;
      }

    case READ_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	uint16_t data = m->data;
	uint8_t result = m->status;
	if (result != SOS_OK)
	  {
	    colorResult.result = -EINVAL;
	    post_long (ADCM1700_CONTROL_PID, ADCM1700_STAT_PID, GET_SUMS_DONE,
		       sizeof (color16_t) + sizeof (uint8_t), &colorResult,
		       0);
	    return SOS_OK;
	  }

	switch (addr)
	  {
	  case ADCM_REG_SUM_GRN1:
	    colorResult.values.green = data >> 1;
	    readRegister (ADCM1700_STAT_PID, ADCM_REG_SUM_RED);
	    break;
	  case ADCM_REG_SUM_RED:
	    colorResult.values.red = data;
	    readRegister (ADCM1700_STAT_PID, ADCM_REG_SUM_BLUE);
	    break;
	  case ADCM_REG_SUM_BLUE:
	    colorResult.values.blue = data;
	    readRegister (ADCM1700_STAT_PID, ADCM_REG_SUM_GRN2);
	    break;
	  case ADCM_REG_SUM_GRN2:
	    colorResult.values.green = colorResult.values.green + (data >> 1);
	    colorResult.result = SOS_OK;
	    post_long (ADCM1700_CONTROL_PID, ADCM1700_STAT_PID, GET_SUMS_DONE,
		       sizeof (color16_t) + sizeof (uint8_t), &colorResult,
		       0);
	    break;
	  default:
	    colorResult.result = -EINVAL;
	    post_long (ADCM1700_CONTROL_PID, ADCM1700_STAT_PID, GET_SUMS_DONE,
		       sizeof (color16_t) + sizeof (uint8_t), &colorResult,
		       0);
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


//***********************Format Control*********************/
int8_t
adcm1700CtrlStatGetSums ()
{
  // The following registers are present only in the second revision 
  // of the ADCM-1700. The register value is equal to the sum of all
  // of the pixels (of that color) in the input window, divided by 16384.
  // Obviously this function is not usefull when there are few pixels in
  // the input window. To get the average pixel value, divide by (nx * ny / 4).

  readRegister (ADCM1700_STAT_PID, ADCM_REG_SUM_GRN1);
  return SOS_OK;
}
