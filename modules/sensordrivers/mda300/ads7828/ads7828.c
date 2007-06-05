/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * ADS7828 Driver Module
 *
 * Gets readings from the ADS7828
 * 
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \author Mike Buchanan
 * \author Ben Jencks
 */

#include <module.h>
#include <sys_module.h>
#include "ads7828.h"
#include <led_dbg.h>

// High level names of the functionality provided by the ADS7828
#include <../drivers/adc/ads7828/include/ads7828.h>
#include <i2c_system.h>

////
// Local enums and defines
////

#define ADS7828_I2C_ADDR 0x4a

////
// Hardware specific defines
////

#define ADS7828_PORT PORTC
#define ADS7828_DIRECTION DDRC

////
// Module specific state and header
////

/**
 * Module state.  Yippie skippie.
 */
typedef struct {
  uint8_t calling_module;
  uint8_t* command;
  uint8_t channel;
} ads7828_state_t;


/**
 * Special module message handler
 */
static int8_t ads7828_msg_handler(void *state, Message *msg);


/**
 * Module header.  Um, yay.
 */
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id        = ADS7828_SENSOR_PID,
  .state_size    = sizeof(ads7828_state_t),
  .num_sub_func  = 0,
  .num_prov_func = 0,
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,
  .code_id       = ehtons(ADS7828_SENSOR_PID),
  .module_handler = ads7828_msg_handler,
};


////
// Local function prototypes
////


////
// Code.  Finally, code.
////

/** Main message handler for ADS7828
 */
static int8_t ads7828_msg_handler(void *state, Message *msg) { 
    
    ads7828_state_t *s = (ads7828_state_t *)state;

    /**
     * <h2>Types of messages handled by the ADS7828 module</h2>
     */
    switch(msg->type) 
	  {
		
        /**
         * \par MSG_INIT:
         * Start a periodic timer that will trigger a sample request from the
         * ADS7828
         */
	  case MSG_INIT:
            {
			  s->calling_module = 0;
			  s->channel = 0;
			  				
			  sys_led(LED_RED_OFF);
			  sys_led(LED_GREEN_OFF);
			  sys_led(LED_YELLOW_OFF);
			  
			  break;
            }
			
            
            /**
             * \par MSG_FINAL:
             * Okay. . . tell someone who cares.
             */
	  case MSG_FINAL:
            { break; }
			
			/**
			 * Turns things on
			 */
	  case ADS_POWER_ON:
		{
		  uint8_t* power_mask = sys_msg_take_data(msg);

		  HAS_CRITICAL_SECTION;
		  ENTER_CRITICAL_SECTION();
		  ADS7828_DIRECTION = ADS7828_DIRECTION | *power_mask;
		  ADS7828_PORT = ADS7828_PORT | *power_mask;
		  LEAVE_CRITICAL_SECTION();
		  
		  break;
		}

			/**
			 *	Gets data from a specific channel.
			 */		
	  case ADS_GET_FROM:
		{
		  uint8_t* chtemp = sys_msg_take_data(msg);
		  s->channel = *chtemp;
		  sys_free(chtemp);
		  //*******************************************************
		  //***Help, I've fallen (through) and I can't get up!*****
		  //*******************************************************
		}
		/**
		 * Send out an I2C request to the ADS7828 for a conversion on
		 * the last channel specified by GET_FROM
		 */
	  case ADS_GET_DATA:
		{
		  s->calling_module = msg->sid;
		  // Try to reserve the I2C bus
		  if(ker_i2c_reserve_bus(ADS7828_SENSOR_PID, I2C_ADDRESS, 
							I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) != SOS_OK) {
		
			return -EBUSY;
		  }
		  
		  // Send a request to configure ADC for a read from CH0
		  s->command = (uint8_t *)sys_malloc(1);
		  
		  // Power down when done (00)
		  s->command[0] = s->channel | ADS7828_PD0 | ADS7828_PD1; 
		  
		  if(ker_i2c_send_data(ADS7828_I2C_ADDR,
							   s->command,
							   1,
							   ADS7828_SENSOR_PID) != SOS_OK) {
			sys_free(s->command);
			ker_i2c_release_bus(ADS7828_SENSOR_PID);
			return -EBUSY;
		  }
		  break;
		}
		
		
		/**
		 * Request has been sent.  Now read the data from the device.  The
		 * device is still reserved, so we are good to go.
		 */
	  case MSG_I2C_SEND_DONE:
		{
		  // Clean up...
                sys_free(s->command);
                
                // Re-reserve the I2C bus in read mode
                if(ker_i2c_reserve_bus(ADS7828_SENSOR_PID, 
									   I2C_ADDRESS, 
									   I2C_SYS_MASTER_FLAG) != SOS_OK) {
                    return -EBUSY;
                }
				
                if(ker_i2c_read_data(ADS7828_I2C_ADDR,2,ADS7828_SENSOR_PID) !=SOS_OK) {
                    ker_i2c_release_bus(ADS7828_SENSOR_PID);
                    return -EBUSY;
                }
                    
                //ker_led(LED_YELLOW_TOGGLE);
                break;
		}
        
		
            /**
             * Read the data!
             */
	  case MSG_I2C_READ_DONE:
		{   
                uint8_t length;
                uint8_t *data;
                uint8_t i;
				
                length = msg->len;
                data = sys_malloc(length);
				
                for(i=0; i<length; i++) {
				  data[i] = ((uint8_t *)(msg->data))[i];
                }
                    
                post_long(s->calling_module, ADS7828_SENSOR_PID, ADS_DATA,
						  length, data, SOS_MSG_RELEASE);

                ker_i2c_release_bus(ADS7828_SENSOR_PID);
				break;
		}
		
		
	  default:
            {
			  return -EINVAL;
            }
	  }
	
    return SOS_OK;
	
}

#ifndef _MODULE_
mod_header_ptr ads7828_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif




