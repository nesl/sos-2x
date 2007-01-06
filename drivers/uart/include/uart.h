
/**
 * @brief header file for UART0
 * @auther Simon Han
 * 
 */

#ifndef _UART_H_
#define _UART_H_


#ifndef _MODULE_
#include <sos_types.h>

void uart_init(void);
uint8_t uart_getState(uint8_t flags);

int8_t uart_initTransceiver(
		uint8_t flags);

int8_t uart_startTransceiverTx(
		uint8_t *msg,
		uint8_t msg_len,
		uint8_t flags);

int8_t uart_startTransceiverRx(
		uint8_t rx_len,
		uint8_t flags);

uint8_t *uart_getRecievedData(void);

#endif /* _MODULE_ */

#endif // _UART_H_
