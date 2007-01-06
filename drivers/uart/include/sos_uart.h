
/**
 * @brief header file for UART0
 * @auther Simon Han
 * 
 */

#ifndef _SOS_UART_H_
#define _SOS_UART_H_


#ifndef _MODULE_
#include <sos_types.h>

#ifndef QUALNET_PLATFORM

/**
 * init function
 */

#ifndef DISABLE_UART
#ifndef NO_SOS_UART
extern void sos_uart_init(void);
#else
#define sos_uart_init()
#endif
#else
#define sos_uart_init()
#endif


/**
 * @brief allocate uart message
 * @return Message poiner or NULL for failure
 */
#ifndef DISABLE_UART
#ifndef NO_SOS_UART
extern void uart_msg_alloc(Message *e);
#else
#define uart_msg_alloc(e) 
#endif
#else
#define uart_msg_alloc(e) 
#endif

#endif //QUALNET_PLATFORM
#endif /* _MODULE_ */

#endif
