
/**
 * @brief    header file for random
 * @author   Simon Han
 */

#ifndef _RANDOM_H
#define _RANDOM_H

#ifndef _MODULE_
#include <sos_types.h>

/**
 * init function
 */
extern void random_init(void);

/**
 * Module API
 * Same as kernel API
 */

/**
 * Kernel API
 */
extern uint16_t ker_rand(void);


#endif /* _MODULE_ */

#endif /* _RANDOM_H */
