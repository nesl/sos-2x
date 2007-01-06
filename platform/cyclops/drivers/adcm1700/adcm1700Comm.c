#include <module.h>
#include <i2c_system.h>
#include <hardware.h>
#include <adcm1700Const.h>

#define IMAGER_I2C_ADDR 0x51

static int8_t adcm1700_comm_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
 mod_id:ADCM1700_COMM_PID,
 state_size:0,
 num_timers:0,
 num_sub_func:0,
 num_prov_func:0,
 module_handler:adcm1700_comm_handler,
};

enum
  {
    //Note: state name always infer the action to be done in next step.
    IMAGER_IDLE = 1,
    //writing happens in two states that is two writing
    IMAGER_WRITING_SET_BLOCK = 2,
    IMAGER_WRITING_DATA = 3,
    //reading happens in three states that is two writing and one reading
    IMAGER_READING_SET_BLOCK = 4,
    IMAGER_READING_SET_OFFSET = 5,
    IMAGER_READING_DATA = 6,
    IMAGER_WRITING_READING_DATA = 7
  };

#define INVALID_ADDRESS 0xff	//we know that the lsb of both Block and offset are always zero
#define MAX_PAYLOAD_SIZE 4

//the state of the camera in terms of block and offset registers, current data register and its content.
static uint8_t currentBlock;
static uint8_t currentOffset;
static uint16_t currentReg;
static uint16_t currentData;
static uint8_t cameraPayload[MAX_PAYLOAD_SIZE];
static uint8_t imagerState;
static uint8_t addr;
struct register_s *cameraRegister;
static register_result_t regResult;
static block_result_t blockResult;

//***********************************************************************************
//****************************Initialization and Termination ************************
//***********************************************************************************
int8_t
adcm1700ImagerCommInit ()
{
  imagerState = IMAGER_IDLE;
  currentBlock = INVALID_ADDRESS;
  currentOffset = INVALID_ADDRESS;
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_comm_handler (void *state, Message * msg)
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
    case MSG_I2C_READ_DONE:
      {
	uint8_t *data = msg->data;
	uint8_t msb, lsb;
	//Kevin - Hardware I2C                    
	ker_i2c_release_bus (ADCM1700_COMM_PID);
	imagerState = IMAGER_IDLE;	// *** Change state BEFORE signal!
	if (GET_REGISTER_LENGHT (currentReg) == TWO_BYTE_REGISTER)
	  {
	    lsb = *data;
	    msb = *(data + 1);
	    currentData =
	      ((((uint16_t) msb) << 8) & 0xff00) +
	      (((uint16_t) lsb) & 0x00ff);
	    //make a copy before posting
	    regResult.reg = currentReg;
	    regResult.data = currentData;
	    regResult.status = SOS_OK;
	    post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
		       sizeof (regResult), &regResult, 0);
	  }
	else if (GET_REGISTER_LENGHT (currentReg) == ONE_BYTE_REGISTER)
	  {
	    currentData = (uint16_t) (*data & 0x00ff);
	    regResult.reg = currentReg;
	    regResult.data = currentData;
	    regResult.status = SOS_OK;
	    post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
		       sizeof (regResult), &regResult, 0);
	  }
	return SOS_OK;
      }
    case MSG_I2C_SEND_DONE:
      {
	MsgParam *p = (MsgParam *) (msg->data);
	bool r = (bool) (p->byte);
	if (r == SOS_OK)
	  {
	    HAS_CRITICAL_SECTION;
	    switch (imagerState)
	      {
		//now that we have written new block we should write actual data.
	      case IMAGER_WRITING_SET_BLOCK:
		currentBlock = GET_REGISTER_BLOCK (currentReg);
		// This critical section ensures integrity at the hardware level...
		ENTER_CRITICAL_SECTION ();
		{
		  imagerState = IMAGER_WRITING_DATA;
		}
		LEAVE_CRITICAL_SECTION ();
					  
		cameraPayload[0] =
		  (((GET_REGISTER_OFFSET (currentReg)) << 1) & 0xfe) + 0x0;
		if (GET_REGISTER_LENGHT (currentReg) == TWO_BYTE_REGISTER)
		  {
		    cameraPayload[1] = (uint8_t) (currentData & 0x00ff);	//lsb of data
		    cameraPayload[2] = (uint8_t) ((currentData >> 8) & 0x00ff);	//msb of data
		    //Kevin - Hardware I2C                                                                  
		    if (ker_i2c_send_data
			(IMAGER_I2C_ADDR, cameraPayload, 3,
			 ADCM1700_COMM_PID) != SOS_OK)
		      {
			regResult.reg = currentReg;
			regResult.data = currentData;
			regResult.status = -EINVAL;
			post_long (addr, ADCM1700_COMM_PID,
				   WRITE_REGISTER_DONE, sizeof (regResult),
				   &regResult, 0);
		      }
		  }
		else if (GET_REGISTER_LENGHT (currentReg) == ONE_BYTE_REGISTER)
		  {
		    // TYPO!!! DISCOVERED 3/24/05 ==> cameraPayload[1]=(uint8_t) ( currentReg & 0x00ff);  //1 byte data
		    cameraPayload[1] = (uint8_t) (currentData & 0x00ff);	//1 byte data                                                                                                            
		    //Hardware I2C
						  
		    if (ker_i2c_send_data
			(IMAGER_I2C_ADDR, cameraPayload, 2,
			 ADCM1700_COMM_PID) != SOS_OK)
		      {
			regResult.reg = currentReg;
			regResult.data = currentData;
			regResult.status = -EINVAL;
			post_long (addr, ADCM1700_COMM_PID,
				   WRITE_REGISTER_DONE, sizeof (regResult),
				   &regResult, 0);
		      }
		  }
		//now that data is also written, do not wait, give signal to user
		break;
	      case IMAGER_WRITING_DATA:
		//currentReg++;
		//  atomic{imagerState = IMAGER_IDLE;}                            
		ENTER_CRITICAL_SECTION ();
		{		//Kevin - Hardware I2C 
		  ker_i2c_release_bus (ADCM1700_COMM_PID);
		  imagerState = IMAGER_IDLE;
		}
		LEAVE_CRITICAL_SECTION ();
		regResult.reg = currentReg;
		regResult.data = currentData;
		regResult.status = SOS_OK;
		post_long (addr, ADCM1700_COMM_PID, WRITE_REGISTER_DONE,
			   sizeof (regResult), &regResult, 0);
		break;
	      case IMAGER_READING_SET_BLOCK:
		currentBlock = GET_REGISTER_BLOCK (currentReg);
		imagerState = IMAGER_READING_SET_OFFSET;
		cameraPayload[0] =
		  (((GET_REGISTER_OFFSET (currentReg)) << 1) & 0xfe) + 0x1;
					  
		//Kevin - Hardware I2C                                            
		if (ker_i2c_send_data
		    (IMAGER_I2C_ADDR, cameraPayload, 1,
		     ADCM1700_COMM_PID) != SOS_OK)
		  {
		    regResult.reg = currentReg;
		    regResult.data = currentData;
		    regResult.status = -EINVAL;
		    post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
			       sizeof (regResult), &regResult, 0);
		  }
		break;
	      case IMAGER_READING_SET_OFFSET:
		imagerState = IMAGER_READING_DATA;
		// grab the bus for reading
		if (ker_i2c_reserve_bus
		    (ADCM1700_COMM_PID, I2C_ADDRESS,
		     I2C_SYS_MASTER_FLAG) != SOS_OK)
		  {
		    ker_i2c_release_bus (ADCM1700_COMM_PID);
		    return -EBUSY;
		  }
		if (GET_REGISTER_LENGHT (currentReg) == TWO_BYTE_REGISTER)
		  {
		    //Kevin - Hardware I2C                                                                
		    if (ker_i2c_read_data
			(IMAGER_I2C_ADDR, 2, ADCM1700_COMM_PID) != SOS_OK)
		      {
			regResult.reg = currentReg;
			regResult.data = currentData;
			regResult.status = -EINVAL;
			post_long (addr, ADCM1700_COMM_PID,
				   READ_REGISTER_DONE, sizeof (regResult),
				   &regResult, 0);
		      }
		  }
		else if (GET_REGISTER_LENGHT (currentReg) == ONE_BYTE_REGISTER)
		  {
		    //Kevin - Hardware I2C                                                                
		    if (ker_i2c_read_data
			(IMAGER_I2C_ADDR, 1, ADCM1700_COMM_PID) != SOS_OK)
		      {
			regResult.reg = currentReg;
			regResult.data = currentData;
			regResult.status = -EINVAL;
			post_long (addr, ADCM1700_COMM_PID,
				   READ_REGISTER_DONE, sizeof (regResult),
				   &regResult, 0);
		      }
		  }
		break;
	      default:
		return -EINVAL;	//never should reach here.
	      }			// state switch
			  
	  }
	else			//unsuccessful writing!
	  {
	    switch (imagerState)
	      {
	      case IMAGER_WRITING_SET_BLOCK:
	      case IMAGER_WRITING_DATA:
		imagerState = IMAGER_IDLE;
		// Kevin - software I2C call
		regResult.reg = currentReg;
		regResult.data = currentData;
		regResult.status = -EINVAL;
		post_long (addr, ADCM1700_COMM_PID, WRITE_REGISTER_DONE,
			   sizeof (regResult), &regResult, 0);
		break;
	      case IMAGER_READING_SET_BLOCK:
	      case IMAGER_READING_SET_OFFSET:
		imagerState = IMAGER_IDLE;
		// Kevin - software I2C call
		regResult.reg = currentReg;
		regResult.data = currentData;
		regResult.status = -EINVAL;
		post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
			   sizeof (regResult), &regResult, 0);
		break;
	      default:
		return -EINVAL;	//never should reach here.
	      }
	    return -EINVAL;	//shouldn't get here either!
	  }
	return SOS_OK;		// program will never reach this point
      }
		  
    case WRITE_BLOCK_DONE:
      {
	return SOS_OK;
      }
    case READ_BLOCK_DONE:
      {
	return SOS_OK;
      }
    case READ_REGISTER_DONE:
      {
	return SOS_OK;
      }
    case WRITE_REGISTER_DONE:
      {
	return SOS_OK;
      }
    default:
      return -EINVAL;
    }
  return SOS_OK;
}

//***********************************************************************************
//******************Writing & Reading Data to/from Imager via I2C********************
//***********************************************************************************
int8_t
writeRegister (sos_pid_t id, uint16_t reg, uint16_t data)
{
  // the next two lines replace the block that has been commented out.
  bool i2cWriteSuccess = false;
  // Kevin - software I2C call
  imagerState = IMAGER_IDLE;

  addr = id;
  currentReg = reg;
  currentData = data;
  //if register is in a different block we should set the block.

  if (GET_REGISTER_BLOCK (reg) != currentBlock)
    {
      imagerState = IMAGER_WRITING_SET_BLOCK;
      cameraPayload[0] = (((BLOCK_SWITCH_CODE) << 1) & 0xfe) + 0x00;
      cameraPayload[1] = GET_REGISTER_BLOCK (reg);
      //Kevin - Hardware I2C              
      if (ker_i2c_reserve_bus
	  (ADCM1700_COMM_PID, I2C_ADDRESS,
	   I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) == SOS_OK)
	{
	  if (ker_i2c_send_data
	      ((IMAGER_I2C_ADDR), cameraPayload, 2,
	       ADCM1700_COMM_PID) == SOS_OK)
	    {
	      i2cWriteSuccess = true;
	    }
	}
      if (!i2cWriteSuccess)	//if we get here, we failed writing to I2C
	{
	  regResult.reg = currentReg;
	  regResult.data = currentData;
	  regResult.status = -EINVAL;
	  post_long (addr, ADCM1700_COMM_PID, WRITE_REGISTER_DONE,
		     sizeof (regResult), &regResult, 0);
	}
    }
  //now that blocks are already set we can only write the actual data
  else
    {
      imagerState = IMAGER_WRITING_DATA;
      cameraPayload[0] = (((GET_REGISTER_OFFSET (reg)) << 1) & 0xfe) + 0x0;
      if (GET_REGISTER_LENGHT (reg) == TWO_BYTE_REGISTER)	//if it is a two byte register 
	{
	  cameraPayload[1] = (uint8_t) (data & 0x00ff);	//lsb of data
	  cameraPayload[2] = (uint8_t) ((data >> 8) & 0x00ff);	//msb of data                                                 
	  if (ker_i2c_reserve_bus
	      (ADCM1700_COMM_PID, I2C_ADDRESS,
	       I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) == SOS_OK)
	    {
	      if (ker_i2c_send_data
		  (IMAGER_I2C_ADDR, cameraPayload, 3,
		   ADCM1700_COMM_PID) == SOS_OK)
		{
		  i2cWriteSuccess = true;
		}
	    }
	  if (!i2cWriteSuccess)	//if we get here, we failed writing to I2C
	    {
	      regResult.reg = currentReg;
	      regResult.data = currentData;
	      regResult.status = -EINVAL;
	      post_long (addr, ADCM1700_COMM_PID, WRITE_REGISTER_DONE,
			 sizeof (regResult), &regResult, 0);
	    }
	}
      else if (GET_REGISTER_LENGHT (reg) == ONE_BYTE_REGISTER)	//if it is a one byte register 
	{
	  cameraPayload[1] = (uint8_t) (data & 0x00ff);	//1 byte data
	  //Kevin - Hardware I2C                                  
	  if (ker_i2c_reserve_bus
	      (ADCM1700_COMM_PID, I2C_ADDRESS,
	       I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) == SOS_OK)
	    {
	      if (ker_i2c_send_data
		  (IMAGER_I2C_ADDR, cameraPayload, 2,
		   ADCM1700_COMM_PID) == SOS_OK)
		{
		  i2cWriteSuccess = true;
		}
	    }
	  if (!i2cWriteSuccess)	//if we get here, we failed writing to I2C
	    {
	      regResult.reg = currentReg;
	      regResult.data = currentData;
	      regResult.status = -EINVAL;
	      post_long (addr, ADCM1700_COMM_PID, WRITE_REGISTER_DONE,
			 sizeof (regResult), &regResult, 0);
	    }
	}
      else
	return -EINVAL;		//we never should reach here.
    }
  return SOS_OK;
}

int8_t
readRegister (sos_pid_t id, uint16_t reg)
{
  bool i2cWriteSuccess = false;
  if (imagerState != IMAGER_IDLE)
    {
      // reset interface to prepare for next transaction
      //Kevin - we dont want to disrupt another I2C transaction, what if a second call came in before the first finished? 
      imagerState = IMAGER_IDLE;
      regResult.reg = reg;
      regResult.data = 0;
      regResult.status = -EINVAL;
      post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
		 sizeof (regResult), &regResult, 0);
      return -EINVAL;
    }
  addr = id;
  currentReg = reg;
  //if register is in a different block we should set the block.
  if (GET_REGISTER_BLOCK (reg) != currentBlock)
    {
      imagerState = IMAGER_READING_SET_BLOCK;
      cameraPayload[0] = (((BLOCK_SWITCH_CODE) << 1) & 0xfe) + 0x00;
      cameraPayload[1] = GET_REGISTER_BLOCK (reg);
      //Kevin - Hardware I2C              
      if (ker_i2c_reserve_bus
	  (ADCM1700_COMM_PID, I2C_ADDRESS,
	   I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) == SOS_OK)
	{
	  if (ker_i2c_send_data
	      (IMAGER_I2C_ADDR, cameraPayload, 2,
	       ADCM1700_COMM_PID) == SOS_OK)
	    {
	      i2cWriteSuccess = true;
	    }
	}
      if (!i2cWriteSuccess)	//if we get here, we failed writing to I2C
	{
	  regResult.reg = currentReg;
	  regResult.data = 0;
	  regResult.status = -EINVAL;
	  post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
		     sizeof (regResult), &regResult, 0);
	}
    }
  //now that blocks are already set we can only read the actual data
  else
    {
      imagerState = IMAGER_READING_SET_OFFSET;
      cameraPayload[0] =
	(((GET_REGISTER_OFFSET (currentReg)) << 1) & 0xfe) + 0x1;
      //Kevin - Hardware I2C              
      if (ker_i2c_reserve_bus
	  (ADCM1700_COMM_PID, I2C_ADDRESS,
	   I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) == SOS_OK)
	{
	  if (ker_i2c_send_data
	      (IMAGER_I2C_ADDR, cameraPayload, 1,
	       ADCM1700_COMM_PID) == SOS_OK)
	    {
	      i2cWriteSuccess = true;
	    }
	}
      if (!i2cWriteSuccess)	//if we get here, we failed writing to I2C
	{
	  regResult.reg = currentReg;
	  regResult.data = 0;
	  regResult.status = -EINVAL;
	  post_long (addr, ADCM1700_COMM_PID, READ_REGISTER_DONE,
		     sizeof (regResult), &regResult, 0);
	}
    }
  return SOS_OK;
}

int8_t
writeBlock (sos_pid_t id, uint16_t startReg, char *data, uint8_t length)
{
  blockResult.startReg = startReg;
  blockResult.data = data;
  blockResult.length = length;
  blockResult.result = SOS_OK;
  post_long (addr, ADCM1700_COMM_PID, WRITE_BLOCK_DONE, sizeof (blockResult),
	     &blockResult, 0);
  return SOS_OK;
}

int8_t
readBlock (sos_pid_t id, uint16_t startReg, char *data, uint8_t length)
{
  blockResult.startReg = startReg;
  blockResult.data = data;
  blockResult.length = length;
  blockResult.result = SOS_OK;
  post_long (addr, ADCM1700_COMM_PID, READ_BLOCK_DONE, sizeof (blockResult),
	     &blockResult, 0);
  return SOS_OK;
}
