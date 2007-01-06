/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/* 
 * Authors: Jaein Jeong, Philip buonadonna
 * Simon Han: port to SOS
 *
 */

/**
 * @author Naim Busek
 */
#include <sos.h>
#include <hardware.h>

#include <spi_hal.h>

int8_t spi_hardware_init() {
	// set PB3(MISO), PB2(MOSI), PB1(SCK), PB0(/SS) as inputs
	DDRB &= ~((1<<DDB3)|(1<<DDB2)|(1<<DDB1)|(1<<DDB0));
	// disable spi interrupts
	SPCR &= ~(1<<SPIE);

	return SOS_OK;
}

int8_t spi_hardware_initMaster() {
	HAS_CRITICAL_SECTION;
	register uint8_t IOReg;

	PORTB |= (1<<PB0);  // set /SS into a safe state
	// set PB2(MOSI), PB1(SCK), PB0(/SS) as outputs
	DDRB |= (1<<DDB2)|(1<<DDB1)|(1<<DDB0);

	ENTER_CRITICAL_SECTION();
	// clear SPIF bit with read from SPSR then SPDR
	IOReg = SPSR;
	IOReg = SPDR;
	// enable SPI in Master Mode with SCK = CK/16
	//SPCR  = (1<<SPIE)|(1<<SPE)|(1<<MSTR)|(1<<SPR0);
	// enable SPI in Master Mode with SCK = CK/4
	SPCR  = (1<<SPIE)|(1<<SPE)|(1<<MSTR);
	LEAVE_CRITICAL_SECTION();
	
	return SOS_OK;
}

