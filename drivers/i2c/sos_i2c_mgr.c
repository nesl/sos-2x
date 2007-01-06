/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief    sos_i2c messaging layer
 * @author	 Naim Busek <ndbusek@gmail.com>
 *
 */
#include <hardware.h>
#include <sos_info.h>
#include <malloc.h>
#include <sos_timer.h>

#include "sos_i2c_mgr.h"

#ifndef NO_SOS_I2C_MGR
//-------------------------------------
// I2C MGR TIMER
//-------------------------------------
#define SOS_I2C_MGR_TID		0
#define SOS_I2C_MGR_INITAL_DELAY       1000L
#define SOS_I2C_MGR_DISCOVERY_INTERVAL 5000L

#define SOS_I2C_MGR_DISCOVERY_MODE  SOS_I2C_MGR_ACTIVE_DISCOVERY

#define DISCOVERY_MSG_LEN 2

enum {
	SOS_I2C_MGR_INIT=0,
	SOS_I2C_MGR_IDLE,
	SOS_I2C_MGR_DISCOVER,
	SOS_I2C_MGR_ERROR,
};
	

// neighbor list element
typedef struct i2c_neighbor {
  uint16_t node_addr;			
  uint8_t i2c_addr;
  struct i2c_neighbor *next;
} i2c_neighbor_t;

// module state
typedef struct sos_i2c_mgr_state {
	uint8_t state;
	i2c_neighbor_t *addrList;
} sos_i2c_mgr_state_t;


static int8_t sos_i2c_mgr_msg_handler(void *state, Message* msg);

static mod_header_t mod_header SOS_MODULE_HEADER ={    
	.mod_id = KER_I2C_MGR_PID,    
	.state_size = sizeof(sos_i2c_mgr_state_t),
	.num_sub_func = 0,   
	.num_prov_func = 0,   
	.module_handler = sos_i2c_mgr_msg_handler,    
};


// sos_i2c_mgr state
sos_i2c_mgr_state_t s;


//-------------------------------------
// INITIALIZE SOS I2C MANAGER
//-------------------------------------
void sos_i2c_mgr_init() {
	ker_register_module(sos_get_header_address(mod_header));
}

//-------------------------------------
// CHECK NEIGHBOR DURING DISPATCH
//-------------------------------------
int8_t check_i2c_address(uint16_t dest_addr) {

	i2c_neighbor_t *addrPtr = s.addrList;

	// return address if found else return error value
	// Discovery NEVER checks the address
	while (addrPtr != NULL ) {
		if (addrPtr->node_addr == dest_addr){	
			return SOS_OK; /* addrPtr->i2c_addr */
		}
		addrPtr = addrPtr->next;
	}	
	return -EINVAL;
}

int8_t add_i2c_address(uint16_t node_addr, uint8_t i2c_addr) {
	
	i2c_neighbor_t *newAddr;
	
	newAddr = ker_malloc(sizeof(i2c_neighbor_t),  KER_I2C_MGR_PID);

	if (NULL != newAddr) {
		newAddr->node_addr = node_addr;
		newAddr->i2c_addr = i2c_addr;
		newAddr->next = s.addrList;
		s.addrList = newAddr;
		
	} else {
		return -ENOMEM;
	}

	return SOS_OK;
}

int8_t rm_i2c_address(uint16_t node_addr, uint8_t i2c_addr) {

	int8_t retVal = -EINVAL;
	
	i2c_neighbor_t *addrPtr = s.addrList;
	i2c_neighbor_t *tmpPtr;

	// handle all instances at the head of the list
	while ((NULL != addrPtr) && ((addrPtr->node_addr == node_addr) || (addrPtr->i2c_addr == i2c_addr))) {
		tmpPtr = addrPtr;
		addrPtr = addrPtr->next;
		ker_free(tmpPtr);
		retVal = SOS_OK;
		
		s.addrList = addrPtr;
	}

	// element is not at the head of the list
	while (NULL != addrPtr->next) {
		if ((addrPtr->next->node_addr == node_addr) || (addrPtr->next->i2c_addr == i2c_addr)) {
			tmpPtr = addrPtr->next;
			addrPtr->next = addrPtr->next->next;
			ker_free(tmpPtr);
			retVal = SOS_OK;
		}
		addrPtr = addrPtr->next;
	}
	return retVal;
}

int8_t send_discovery_msg(uint16_t i2c_addr, uint8_t discover_msg_type) {
	uint8_t *data;

	data = ker_malloc(DISCOVERY_MSG_LEN, KER_I2C_MGR_PID);

	if (NULL != data) {
		data[0] = ker_i2c_id();
		data[1] = discover_msg_type;
		if (SOS_OK ==
				post_i2c(KER_I2C_MGR_PID,
					KER_I2C_MGR_PID,
					MSG_DISCOVERY,
					DISCOVERY_MSG_LEN,
					data,
					SOS_MSG_RELEASE|SOS_MSG_RELIABLE,
					i2c_addr)) {
		} else {
			ker_free(data);
			return -EIO;
		}
	} else {
		return -ENOMEM;
	}

	return SOS_OK;
}
//-------------------------------------
// MESSAGE HANDLER
//-------------------------------------
static int8_t sos_i2c_mgr_msg_handler(void *state, Message* msg) {

	switch (msg->type){
	case MSG_INIT:
		if (SOS_I2C_MGR_DISCOVERY_MODE == SOS_I2C_MGR_ACTIVE_DISCOVERY) {
			ker_timer_init(KER_I2C_MGR_PID, SOS_I2C_MGR_TID, TIMER_ONE_SHOT);
			ker_timer_start(KER_I2C_MGR_PID, SOS_I2C_MGR_TID, SOS_I2C_MGR_INITAL_DELAY);
			s.addrList = NULL;
			s.state = SOS_I2C_MGR_INIT;
		}
		break;

	case MSG_TIMER_TIMEOUT:
		{
			switch (s.state) {
				case SOS_I2C_MGR_INIT:
					ker_timer_init(KER_I2C_MGR_PID, SOS_I2C_MGR_TID, TIMER_REPEAT);
					ker_timer_start(KER_I2C_MGR_PID, SOS_I2C_MGR_TID, SOS_I2C_MGR_DISCOVERY_INTERVAL);
					// fall through

				case SOS_I2C_MGR_IDLE:
					{
						int8_t retVal;

						if (SOS_OK != (retVal = send_discovery_msg(BCAST_ADDRESS, SOS_I2C_MGR_TYPE_DISCOVERY))) {
							return retVal;
						} else {
							s.state = SOS_I2C_MGR_DISCOVER;
						}
					}
					break;

				case SOS_I2C_MGR_DISCOVER:
					// send done failed reset state and wait for next timeout
					s.state = SOS_I2C_MGR_IDLE;
					break;
					
				case SOS_I2C_MGR_ERROR:
				default:
					break;
			}
			break;
		}
		break;

	case MSG_I2C_SEND_DONE:
		s.state = SOS_I2C_MGR_IDLE;
		// Need to fill this out with recovery
		// logic when send of discovery pkt fails
		break;

	case MSG_DISCOVERY:
		{
			uint16_t node_addr;
			uint8_t i2c_addr;

			node_addr = msg->saddr;
			i2c_addr = msg->data[0];
			
			// If we dont already have the entry, add it
			if( check_i2c_address(node_addr) != SOS_OK )
			{				
				add_i2c_address(node_addr, i2c_addr);
			}

			if ((SOS_I2C_MGR_DISCOVERY_MODE == SOS_I2C_MGR_PASSIVE_DISCOVERY) && 
					(msg->data[1] == SOS_I2C_MGR_TYPE_DISCOVERY)){
				if (SOS_OK == send_discovery_msg(i2c_addr, SOS_I2C_MGR_TYPE_REPLY)) {
					s.state = SOS_I2C_MGR_DISCOVER;
				}
			}
		}
		break;

	default:
		break;
	}
	return SOS_OK;
}

#endif // NO_SOS_I2C_MGR

