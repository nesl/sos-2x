/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
      
/**
 * @brief header file for I2C Manager
 * @author Simon Han
 * 
 */

#ifndef _SOS_I2C_MRG_H_
#define _SOS_I2C_MGR_H_


#ifndef _MODULE_
#include <sos_types.h>

#define SOS_I2C_MGR_ACTIVE_DISCOVERY  0x01
#define SOS_I2C_MGR_PASSIVE_DISCOVERY 0x00

enum {
	SOS_I2C_MGR_TYPE_DISCOVERY=0,
	SOS_I2C_MGR_TYPE_ADVERTISMENT,
	SOS_I2C_MGR_TYPE_REPLY,
};
	

#ifndef NO_SOS_I2C_MGR
// Init function
extern void sos_i2c_mgr_init(void);
// locate i2c addr in LUT given node addr
// return invalid address in the 0x78 range if not found
extern int8_t check_i2c_address(uint16_t addr);
// return the i2c address from the lookup table
// NOTE: the I2C_BCAST addr may be associated with any node addr
// if the lookup fails the I2C_BCAST addr will be returned
extern int8_t lookup_i2c_address(uint16_t addr);
// add address when discovery message is recieved
extern int8_t add_i2c_address(uint16_t node_addr, uint8_t i2c_addr);
// remove address after disconnect message or timeout
extern int8_t rm_i2c_address(uint16_t node_addr, uint8_t i2c_addr);
#else
// this may need to init and shut off manager
#define sos_i2c_mgr_init()
// no i2c so no valid matches occure 
#define check_i2c_address(...) -EINVAL
// no valid matches always return the broadcast address
#define lookup_i2c_address(...) I2C_BCAST_ADDRESS
// all adds and removes fail
#define add_i2c_address(...) -EINVAL
#define rm_i2c_address(...) -EINVAL
#endif // NO_SOS_I2C_MGR

#endif /* _MODULE_ */

#endif // _SOS_I2C_MGR_H_
