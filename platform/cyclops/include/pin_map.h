/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
#ifndef _PIN_MAP_H
#define _PIN_MAP_H

#include <pin_defs.h>
#include <pin_alt_func.h>

/*
#define INT_ENABLE()  sbi(EIMSK , 7)  //Interrupt definition.INT7 of MCU 
#define INT_DISABLE() cbi(EIMSK , 7)
#define MAKE_PORTA_OUTPUT() DDRA=0xff
#define MAKE_SRAM_60K() XMCRB=((0<<XMM2) | (0<<XMM1) | (0<<XMM0))                //make memory space 60KB. 
#define SET_RAM_CONTROL_LINES_IN_SAFE_MODE() cbi(DDRG, 0); cbi(DDRG, 1); sbi(PORTG, 0); sbi(PORTG, 1); cbi(XMCRB,XMBK) //make RD,WR input pins and disable the bus keeper 
*/

// LED assignments
ALIAS_IO_PIN(RED_LED, PINF0);
ALIAS_IO_PIN(GREEN_LED, PINF1);
ALIAS_IO_PIN(YELLOW_LED, PINF2);
ALIAS_IO_PIN(AMBER_LED, PINF3);

ALIAS_IO_PIN(AMP_SHDN, PINB4);
// Camera Enable - Active Low Line
ALIAS_IO_PIN(CAMERA_EN, PINB5); 
ALIAS_IO_PIN(CPLD_CLOCK_EN, PINB6);			// CPLD CLOCK (active high) 
// Memory and CPLD - Active Low Line
ALIAS_IO_PIN(MEM_CPLD_EN, PINB7);		// VOLTAGE_REGULATOR (active low)

// software i2c bus assignments
// these will be obsoleated shortly and
// the device will use hw i2c exclusivly
ALIAS_IO_PIN(SW_I2C_SCL, PIND6);
ALIAS_IO_PIN(SW_I2C_SDA, PIND7);

// TRIG for interrupting cyclops
// Used for debugging (monitor Vcc))
ALIAS_IO_PIN(TRIG_EN, PIND2);

// cpld control assignments
ALIAS_IO_PIN(ADDRESS_LATCH_OUTPUT_EN, PINE2);		// ADDRESS latch (active low)
ALIAS_IO_PIN(HS0_CPLD_RUN_EN, PINE5); 				// LOW= CPLD Program mode, HIGH = CPLD Run mode
ALIAS_IO_PIN(HS1_SHFT_REG_CLK, PINE6);
ALIAS_IO_PIN(HS2_STATUS_PIN, PINE7);					// HS2 status pin, used for interrupts
ALIAS_IO_PIN(HS3_MCLK_EN, PINE4);					// (Camera Module clock control ( Active high )
ALIAS_IO_PIN(HS4, PINB0);

ALIAS_IO_PIN(SRAM_WR_PIN, PING0);
ALIAS_IO_PIN(SRAM_RD_PIN, PING1);

// each nop is 1 clock cycle
// 1 clock cycle on mica2 == 136ns
static inline void TOSH_wait_250ns() {         
	asm volatile  ("nop" ::);     
	asm volatile  ("nop" ::);     
}

static inline void TOSH_uwait(uint16_t _u_sec) {        
	while (_u_sec > 0) {            
		/* XXX SIX nop will give 8 cycles, which is 1088 ns */
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		asm volatile  ("nop" ::);     
		_u_sec--;                     
	}                               
}

void init_IO(void);

#endif //_PIN_MAP_H

