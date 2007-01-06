
/**
 * @brief header file for UART0
 * @auther Simon Han
 * 
 */

#ifndef _SOS_UART_MRG_H_
#define _SOS_UART_MGR_H_


#ifndef _MODULE_
#include <sos_types.h>

/**
 * init function
 */

#ifndef NO_SOS_UART_MGR
extern void sos_uart_mgr_init(void);
#else
#define sos_uart_mgr_init()
#endif

/**
 * @brief add/remove address from lookup table
 * @return Should return -ENOMEM on failure
 */
#ifndef NO_SOS_UART_MGR
/**
 * @brief add/remove address from lookup table
 * @return SOS_OK if found or -EINVAL
 */
extern int8_t check_uart_address(uint16_t addr);

extern void add_uart_addr(uint16_t addr);
extern void rm_uart_addr(uint16_t addr);

extern void set_uart_address(uint16_t addr); 
#else
#define check_uart_address(addr) ((addr != UART_ADDRESS)?-EINVAL:SOS_OK)

#define add_uart_addr(...) -EINVAL
#define rm_uart_addr(...) -EINVAL

#define set_uart_address(...) -EINVAL
#endif

#endif /* _MODULE_ */

#endif
