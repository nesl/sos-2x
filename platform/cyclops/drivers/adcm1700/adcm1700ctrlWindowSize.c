#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlWindowSize.h>
#include <memConst.h>
#include <image.h>

static int8_t adcm1700_window_size_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id:ADCM1700_WINDOW_SIZE_PID,
state_size:0,
num_timers:0,
num_sub_func:0,
num_prov_func:0,
module_handler:adcm1700_window_size_handler,
};

/* internal structure for the window sizes */
static wsize_t currentInputSize;
static wsize_t currentOutputSize;
static wpos_t currentInputOffset;
static int16_t position;	// working variable for window position calculations
static uint8_t currentCommand;

enum				// for current command
{
	IN_SIZE,
	OUT_SIZE,
	IN_PAN
};

//*******************Inititialization and Termination*********************/
int8_t
adcm1700WindowSizeInit ()
{
	// illegal values force initialization
	currentInputSize.x = 0;
	currentInputSize.y = 0;
	currentOutputSize.x = 0;
	currentOutputSize.y = 0;
	currentInputOffset.x = 0;
	currentInputOffset.y = 0;
	ker_register_module (sos_get_header_address (mod_header));
	return SOS_OK;
}

static int8_t
adcm1700_window_size_handler (void *state, Message * msg)
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
		case SET_INPUT_SIZE_DONE:
		{
			return SOS_OK;
		}
		case SET_OUTPUT_SIZE_DONE:
		{
			return SOS_OK;
		}
		case SET_INPUT_PAN_DONE:
		{
			return SOS_OK;
		}
			
			//***********************Writing and Reading individual Bytes*********************/
		case READ_REGISTER_DONE:
		{
			register_result_t *m = (register_result_t *) (msg->data);
			uint16_t addr = m->reg;
			uint16_t data = m->data;
			uint8_t result = m->status;
			if (result != SOS_OK)
			{
				switch (currentCommand)
				{
					case IN_SIZE:
						post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
									SET_INPUT_SIZE_DONE, -EINVAL, 0, 0);
						break;
					case OUT_SIZE:
						post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
									SET_OUTPUT_SIZE_DONE, -EINVAL, 0, 0);
						break;
					case IN_PAN:
						post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
									SET_INPUT_PAN_DONE, -EINVAL, 0, 0);
						break;
				}
				return -EINVAL;
			}
			switch (addr)
			{
				case ADCM_REG_PROC_CTRL_V:	// make sure sizer bit is zero
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_PROC_CTRL_V,
								   (data & ~ADCM_PROC_CTRL_NOSIZER));
					break;
				default:
					// ??? some other register ???
					break;
			}
			
			return SOS_OK;
		}
		case WRITE_REGISTER_DONE:
		{
			register_result_t *m = (register_result_t *) (msg->data);
			uint16_t addr = m->reg;
			//uint16_t data = m->data;
			uint8_t result = m->status;
			//if (status != SOS_OK) signal windowSize.setWindowSizeDone(-EINVAL);
			switch (addr)
			{
				// sizer input window size
				case ADCM_REG_SZR_IN_WID_V:
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_SZR_IN_HGT_V,
								   currentInputSize.y);
					break;
				case ADCM_REG_SZR_IN_HGT_V:
					post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
								SET_INPUT_SIZE_DONE, result, 0, 0);
					break;
					
					// sizer output window size
				case ADCM_REG_SZR_OUT_WID_V:
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_SZR_OUT_HGT_V,
								   currentOutputSize.y);
					break;
				case ADCM_REG_SZR_OUT_HGT_V:
					readRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_PROC_CTRL_V);	// activate sizer
					break;
				case ADCM_REG_PROC_CTRL_V:
					post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
								SET_OUTPUT_SIZE_DONE, result, 0, 0);
					break;
					
					// input window coordinates (for panning)
				case ADCM_REG_FWCOL:
					position =
					(((ADCM_SIZE_MAX_X + currentInputSize.x) >> 1) +
					 currentInputOffset.x + 8) >> 2;
					//telegraph16(position);
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_LWCOL,
								   (uint8_t) position);
					break;
				case ADCM_REG_LWCOL:
					position =
					(((ADCM_SIZE_MAX_Y - currentInputSize.y) >> 1) +
					 currentInputOffset.y + 4) >> 2;
					//telegraph16(position);
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_FWROW,
								   (uint8_t) position);
					break;
				case ADCM_REG_FWROW:
					position =
					(((ADCM_SIZE_MAX_Y + currentInputSize.y) >> 1) +
					 currentInputOffset.y + 8) >> 2;
					//telegraph16(position);
					writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_LWROW,
								   (uint8_t) position);
					break;
				case ADCM_REG_LWROW:
					post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
								SET_INPUT_PAN_DONE, SOS_OK, 0, 0);
					break;
					
					
				default:		// some other register ???
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


// interface commands
int8_t
adcm1700WindowSizeSetInputSize (wsize_t iwsize)
{
	currentCommand = IN_SIZE;
	
	// enforce 4-byte alignment requirement
	iwsize.x = (iwsize.x >> 2) << 2;
	iwsize.y = (iwsize.y >> 2) << 2;
	
	// check limits 
	if (iwsize.x > ADCM_SIZE_MAX_X)
		iwsize.x = ADCM_SIZE_MAX_X;
	
	if (iwsize.x < ADCM_SIZE_MIN_X)
		iwsize.x = ADCM_SIZE_MIN_X;
	
	if (iwsize.y > ADCM_SIZE_MAX_Y)
		iwsize.y = ADCM_SIZE_MAX_Y;
	
	if (iwsize.y < ADCM_SIZE_MIN_Y)
		iwsize.y = ADCM_SIZE_MIN_Y;
	
	if ((iwsize.x != currentInputSize.x) || (iwsize.y != currentInputSize.y))
    {
		currentInputSize.x = iwsize.x;
		currentInputSize.y = iwsize.y;
		
		// set input size register ...
		writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_SZR_IN_WID_V,
					   currentInputSize.x);
    }
	else				// no need to do anything
		post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
					SET_INPUT_SIZE_DONE, SOS_OK, 0, 0);
	return SOS_OK;
}

int8_t
adcm1700WindowSizeSetOutputSize (wsize_t owsize)
{
	currentCommand = OUT_SIZE;
	
	// enforce 4-byte alignment requirement
	owsize.x = (owsize.x >> 2) << 2;
	owsize.y = (owsize.y >> 2) << 2;
	
	// check limits 
	if (owsize.x > ADCM_SIZE_MAX_X)
		owsize.x = ADCM_SIZE_MAX_X;
	
	if (owsize.x < ADCM_SIZE_MIN_X)
		owsize.x = ADCM_SIZE_MIN_X;
	
	if (owsize.y > ADCM_SIZE_MAX_Y)
		owsize.y = ADCM_SIZE_MAX_Y;
	
	if (owsize.y < ADCM_SIZE_MIN_Y)
		owsize.y = ADCM_SIZE_MIN_Y;
	
	// *** DEBUG ***: for output size to be updated
	if ((owsize.x != currentOutputSize.x) || (owsize.y != currentOutputSize.y))
    {
		currentOutputSize.x = owsize.x;
		currentOutputSize.y = owsize.y;
		
		// set output size register ...
		writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_SZR_OUT_WID_V,
					   currentOutputSize.x);
    }
	else				// no need to do anything
		post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
					SET_OUTPUT_SIZE_DONE, SOS_OK, 0, 0);
	return SOS_OK;
}

int8_t
adcm1700WindowSizeSetInputPan (wpos_t iwpan)
{
	uint8_t dxMax;		// maximum permissable offset
	uint8_t dyMax;
	
	currentCommand = IN_PAN;
	// enforce 4-byte alignment requirement
	iwpan.x = (iwpan.x >> 2) << 2;
	iwpan.y = (iwpan.y >> 2) << 2;
	
	// check limits
	dxMax = (ADCM_SIZE_MAX_X - currentInputSize.x) >> 1;
	dyMax = (ADCM_SIZE_MAX_Y - currentInputSize.y) >> 1;
	
	if (iwpan.x < 0)		// x-offset check
    {
		if (iwpan.x < -dxMax)
			iwpan.x = -dxMax;
    }
	else
    {
		if (iwpan.x > dxMax)
			iwpan.x = dxMax;
    }
	
	if (iwpan.y < 0)		// y-offset check
    {
		if (iwpan.y < -dyMax)
			iwpan.y = -dyMax;
    }
	else
    {
		if (iwpan.y > dyMax)
			iwpan.y = dyMax;
    }
	
	// The offset position is lost when the module is stopped, so it must always
	// be reloaded.
	if ((iwpan.x != 0) || (iwpan.y != 0))
    {
		currentInputOffset.x = iwpan.x;
		currentInputOffset.y = iwpan.y;
		
		// set input position register ...
		position =
			(((ADCM_SIZE_MAX_X - currentInputSize.x) >> 1) +
			 currentInputOffset.x + 4) >> 2;
		//telegraph16(position);
		writeRegister (ADCM1700_WINDOW_SIZE_PID, ADCM_REG_FWCOL,
					   (uint8_t) position);
		
    }
	else				// no need to do anything
		post_short (ADCM1700_CONTROL_PID, ADCM1700_WINDOW_SIZE_PID,
					SET_INPUT_PAN_DONE, SOS_OK, 0, 0);
	return SOS_OK;
}

// default callbacks
