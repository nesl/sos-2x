/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief    sos_uart messaging layer
 * @author	 Naim Busek <ndbusek@gmail.com>
 *
 */
#include <sos_uart_mgr.h>
#include <sos_info.h>

static uint16_t uart_address = NODE_ADDRESS;

int8_t check_uart_address(uint16_t addr) {
	if (addr != uart_address) {
		return -EINVAL;
	}
	return SOS_OK;
}

void set_uart_address(uint16_t addr) {
	uart_address = addr;
}
