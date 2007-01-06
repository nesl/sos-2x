/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file spi.h
 * @brief Multimaster SPI SOS interface
 * @author Naim Busek
 *
 * This work is based on the Avr application notes avr151.
 **/

#ifndef _BUS_CB_H_
#define _BUS_CB_H_

typedef int8_t (*bus_read_done_cb_t)(uint8_t *buff, uint8_t bytes, uint8_t count, uint8_t status);
typedef int8_t (*bus_send_done_cb_t)(uint8_t bytes, uint8_t count, uint8_t status);

#endif // _BUS_CB_H_
