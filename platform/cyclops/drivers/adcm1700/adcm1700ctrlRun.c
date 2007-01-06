// Authors : 
//           Rick Baer       rick_baer@agilent.com 
//           Mohammad Rahimi mhr@cens.ucla.edu 
// Ported By :
//           Kevin Lee       kevin79@ucla.edu   
//includes imagerConst;

#include <module.h>
#include <hardware.h>
#include <adcm1700Comm.h>
#include <adcm1700Const.h>
#include <adcm1700ctrlRun.h>
#include <memConst.h>
#include <image.h>

#define POLL_DELAY 50		// delay between polling attempts
#define POLL_MAX 16		// maximum number of polling attempts
#define SENSOR_RUN_BIT 0x02
#define PROC_RUN_BITS 0x70
#define RUN_TID               1

static int8_t adcm1700_run_handler (void *state, Message * e);
static mod_header_t mod_header SOS_MODULE_HEADER = {
 mod_id:ADCM1700_RUN_PID,
 state_size:0,
 num_timers:1,
 num_sub_func:0,
 num_prov_func:0,
 module_handler:adcm1700_run_handler,
};

// register values for commands
enum
  {
    MODULE_RUN = 0x0001,
    MODULE_STOP = 0x0000,
    SENSOR_RUN = 0x24,		// low-power mode remains enabled
    SENSOR_STOP = 0x20
  };

static uint8_t currentModuleState;	// current state
static uint8_t currentSensorState;
static uint8_t currentTarget;	// target of run/stop operation
enum
  {
    TARGET_MODULE,
    TARGET_SENSOR
  };

static uint16_t currentRegister;	// used in polling
static uint8_t pollCount;	// abort after polling MAX_POLL times

//*******************Inititialization and Termination*********************/

int8_t
adcm1700CtrlRunInit ()
{
  ker_register_module (sos_get_header_address (mod_header));
  return SOS_OK;
}

static int8_t
adcm1700_run_handler (void *state, Message * msg)
{
  // i2c_msg_state_t *s = (i2c_msg_state_t*)state;
  switch (msg->type)
    {
    case MSG_INIT:
      {
	currentModuleState = CYCLOPS_RUN;	// the module will begin to run when it is powered up
	currentSensorState = CYCLOPS_RUN;
	return SOS_OK;
      }
    case MSG_FINAL:
      {
	return SOS_OK;
      }
    case MSG_TIMER_TIMEOUT:
      {
	readRegister (ADCM1700_RUN_PID, currentRegister);
	return SOS_OK;
      }
			
    case READ_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	uint16_t data = m->data;
	uint8_t result = m->status;
			
	if (result != SOS_OK)	// failure clause
	  {
	    switch (currentTarget)
	      {
	      case TARGET_MODULE:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
			    CAMERA_DONE, -EINVAL, currentModuleState, 0);
		break;
	      case TARGET_SENSOR:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
			    SENSOR_DONE, -EINVAL, currentSensorState, 0);
		break;
	      default:
		break;
	      }
	    return -EINVAL;
	  }
			
	switch (addr)
	  {
	  case ADCM_REG_SENSOR_CTRL:
	    if (data & SENSOR_RUN_BIT)
	      {
		if (pollCount >= POLL_MAX)
		  {
		    switch (currentTarget)
		      {
		      case TARGET_MODULE:
			post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
				    CAMERA_DONE, -EINVAL, currentModuleState,
				    0);
			break;
		      case TARGET_SENSOR:
			post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
				    SENSOR_DONE, -EINVAL, currentSensorState,
				    0);
			break;
		      default:
			break;
		      }
		    return -EINVAL;
		  }
		currentRegister = ADCM_REG_SENSOR_CTRL;	// polling loop: check every 50 mS                                                                           
		ker_timer_init (ADCM1700_RUN_PID, RUN_TID, TIMER_ONE_SHOT);
		ker_timer_start (ADCM1700_RUN_PID, RUN_TID, POLL_DELAY);
		pollCount++;
	      }
	    else
	      {
		if (currentTarget == TARGET_MODULE)
		  readRegister (ADCM1700_RUN_PID, ADCM_REG_STATUS_FLAGS);	// make sure processor is empty
		else
		  post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
			      SENSOR_DONE, result, currentSensorState, 0);
	      }
	    break;
	  case ADCM_REG_STATUS_FLAGS:
	    if ((data & PROC_RUN_BITS) == PROC_RUN_BITS)
	      post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID, CAMERA_DONE,
			  result, currentModuleState, 0);
	    else
	      {
		if (pollCount >= POLL_MAX)
		  {
		    switch (currentTarget)
		      {
		      case TARGET_MODULE:
			post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
				    CAMERA_DONE, -EINVAL, currentModuleState,
				    0);
			break;
		      case TARGET_SENSOR:
			post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
				    SENSOR_DONE, -EINVAL, currentSensorState,
				    0);
			break;
		      default:
			break;
		      }
		    return -EINVAL;
		  }
		currentRegister = ADCM_REG_STATUS_FLAGS;	// polling loop: check every 50 mS                                                                                                                          
		ker_timer_init (ADCM1700_RUN_PID, RUN_TID, TIMER_ONE_SHOT);
		ker_timer_start (ADCM1700_RUN_PID, RUN_TID, POLL_DELAY);
		pollCount++;
	      }
	    break;
	  }
	return SOS_OK;
      }
    case WRITE_REGISTER_DONE:
      {
	register_result_t *m = (register_result_t *) (msg->data);
	uint16_t addr = m->reg;
	uint16_t data = m->data;
	uint8_t result = m->status;
			
	if (result != SOS_OK)	// failure clause
	  {
	    switch (currentTarget)
	      {
	      case TARGET_MODULE:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
			    CAMERA_DONE, -EINVAL, currentModuleState, 0);
		break;
	      case TARGET_SENSOR:
		post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID,
			    SENSOR_DONE, -EINVAL, currentSensorState, 0);
		break;
	      default:
		break;
	      }
	    return -EINVAL;
	  }
			
	switch (addr)
	  {
	  case ADCM_REG_CONTROL:	// module
	    if (data == MODULE_STOP)
	      readRegister (ADCM1700_RUN_PID, ADCM_REG_SENSOR_CTRL);	// make sure sensor has stopped
	    else
	      post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID, CAMERA_DONE,
			  result, currentModuleState, 0);
	    break;
	  case ADCM_REG_SENSOR_CTRL:	// sensor
	    if (data == SENSOR_STOP)
	      readRegister (ADCM1700_RUN_PID, ADCM_REG_SENSOR_CTRL);	// make sure sensor has stopped 
	    else
	      post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID, SENSOR_DONE,
			  result, currentSensorState, 0);
	    break;
	  default:		// we shouldn't get here
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
    case CAMERA_DONE:
      {
	return SOS_OK;
      }
    case SENSOR_DONE:
      {
	return SOS_OK;
      }
    default:
      return -EINVAL;
    }
  return SOS_OK;
}

//***********************Run Control*********************/
int8_t
adcm1700CtrlRunCamera (uint8_t run_stop)
{
  currentTarget = TARGET_MODULE;
  pollCount = 0;
	
  // *** REMOVED 4/14/05 by RLB ***
  // *** Causes timing problems on the first capture ***
  //if (run_stop == currentModuleState)   // no action required
  //    {
  //        signal run.cameraDone(run_stop, SOS_OK);
  //        return SOS_OK;
  //    }
	
  currentModuleState = run_stop;
  currentSensorState = run_stop;	// sensor is slave to module
	
  if (run_stop == CYCLOPS_RUN)
    writeRegister (ADCM1700_RUN_PID, ADCM_REG_CONTROL, MODULE_RUN);
  else
    writeRegister (ADCM1700_RUN_PID, ADCM_REG_CONTROL, MODULE_STOP);
	
  return SOS_OK;
}

int8_t
adcm1700CtrlRunSensor (uint8_t run_stop)
{
  currentTarget = TARGET_SENSOR;
  pollCount = 0;
	
  if (run_stop == currentSensorState)
    {
      post_short (ADCM1700_CONTROL_PID, ADCM1700_RUN_PID, SENSOR_DONE, SOS_OK,
		  run_stop, 0);
      return SOS_OK;
    }
  currentSensorState = run_stop;
  if (run_stop == CYCLOPS_RUN)
    writeRegister (ADCM1700_RUN_PID, ADCM_REG_SENSOR_CTRL, SENSOR_RUN);
  else
    writeRegister (ADCM1700_RUN_PID, ADCM_REG_SENSOR_CTRL, SENSOR_STOP);
  return SOS_OK;
}
