
/**
 * @brief header file for TWI
 * @auther Simon Han
 * 
 */

#ifndef _SOS_I2C_H_
#define _SOS_I2C_H_


#ifndef _MODULE_
#include <sos_types.h>

#ifndef QUALNET_PLATFORM

/**
 * init function
 */

#ifndef NO_SOS_I2C
extern void sos_i2c_init(void);
#else
#define sos_i2c_init()
#endif

/**
 * Module API
 * Same as kernel api
 */

/**
 * @brief allocate i2c message
 * @return Message poiner or NULL for failure
 */
#ifndef NO_SOS_I2C
extern void i2c_msg_alloc(Message *e);
#else
#define i2c_msg_alloc(e)
#endif

#endif //QUALNET_PLATFORM

#endif /* _MODULE_ */

#endif
