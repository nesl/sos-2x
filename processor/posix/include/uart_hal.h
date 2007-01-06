#ifndef _UART_HAL_H
#define _UART_HAL_H

/**
 * @brief UART0 task
 */
extern void uart_hardware_init(void);
extern void uart_hardware_terminate( void );

extern uint8_t uart_getByte();
extern void uart_setByte(uint8_t b);
#define uart_checkError()          0
#define uart_recv_interrupt()      void uart_recv_int_(void)
#define uart_send_interrupt()      void uart_send_int_(void)
#define uart_disable_rx()        
#define uart_enable_rx()         
extern void uart_enable_tx();
extern void uart_disable_tx();
extern bool uart_is_disabled();

extern void uart_recv_int_(void);
extern void uart_send_int_(void);

#endif // _UART_HAL_H

