#include <message_types.h>
#include "hardware.h"
#include "cpldConst.h"


#define sbi(port,pin) (port |= (1<<(pin))) 
#define cbi(port,pin) (port &= ~(1<<(pin))) 


//****************************HS Lines Functionality************************
//HS0 which is program/run mode pin and is connected to PORTE pin 5
//HS1 which is clock pin is connected to PORTE pin 6
//HS2 is cpld reply pin is connected to PORTE pin 7, it is also interrupt 7 and can be set in interrupt mode.
//HS3 is MCLK control pin, connected to PORTE pin4

//HS0 definition
#define MAKE_MODE_PIN_OUTPUT() SET_HS0_CPLD_RUN_EN_DD_OUT()			// sbi(DDRE, 5)
//#define MAKE_MODE_PIN_INPUT()   cbi(DDRE, 5)
#define SET_CPLD_PROGRAM_MODE() CLR_HS0_CPLD_RUN_EN()				// cbi(PORTE, 5)		
#define SET_CPLD_RUN_MODE() SET_HS0_CPLD_RUN_EN()   					// sbi(PORTE, 5)

//HS1 definition
#define MAKE_CLOCK_PIN_OUTPUT() SET_HS1_SHFT_REG_CLK_DD_OUT() 				// sbi(DDRE, 6)
//#define MAKE_CLOCK_PIN_INPUT()  cbi(DDRE, 6)
#define SET_CLOCK()  SET_HS1_SHFT_REG_CLK()      							// sbi(PORTE, 6)
#define CLEAR_CLOCK() CLR_HS1_SHFT_REG_CLK()								// cbi(PORTE, 6)

//HS2 definition, it is input and it is interrupt pin
#define MAKE_STATUS_PIN_INPUT() SET_HS2_STATUS_PIN_DD_IN()   				// cbi(DDRE, 7)
#define GET_CPLD_STATUS()   ((inp(PINE) >> 7) &0x1)
#define INT_ENABLE()  sbi(EIMSK , 7)  //Interrupt definition.INT7 of MCU 
#define INT_DISABLE() cbi(EIMSK , 7)
#define INT_RISING_EDGE() {\
				sbi(EICRB,ISC71); 	\
				sbi(EICRB,ISC70);	\
} //Note this is asynchronous INT.

// HS3 definition
#define MAKE_MCLK_CTRL_PIN_OUTPUT() SET_HS3_MCLK_EN_DD_OUT()			 // sbi(DDRE, 4)
//#define MAKE_MCLK_CTRL_PIN_INPUT()  cbi(DDRE, 4)
#define TURN_MCLK_ON()	SET_HS3_MCLK_EN()								// sbi(PORTE, 4)
#define TURN_MCLK_OFF() CLR_HS3_MCLK_EN() 							// cbi(PORTE, 4)

// I2C monitoring pins
// The I2C lines must not be configured as outputs when the memory system is shut down
/*
#define MAKE_I2C_CLOCK_PIN_INPUT()  cbi(DDRD, 6)
#define MAKE_I2C_CLOCK_PIN_OUTPUT() sbi(DDRD, 6)
*/
#define SET_I2C_CLOCK_OFF()         cbi(PORTD, 6)
/*
#define MAKE_I2C_DATA_PIN_INPUT()   cbi(DDRD, 7)
#define MAKE_I2C_DATA_PIN_OUTPUT()  sbi(DDRD, 7)
*/
#define SET_I2C_DATA_OFF()          cbi(PORTD, 7)


// CPLD clock control
#define MAKE_CPLD_CLOCK_PIN_OUTPUT() SET_CPLD_CLOCK_EN_DD_OUT() 	// sbi(DDRB, 6)
#define TURN_CPLD_CLOCK_ON() SET_CPLD_CLOCK_EN() 					// sbi(PORTB, 6)  // enable CPLD clock oscillator
#define TURN_CPLD_CLOCK_OFF() CLR_CPLD_CLOCK_EN()					// cbi(PORTB, 6)  // disable CPLD clock oscillator

//****************************SRAM Functionality************************
//NOTE: the fact that RD and most importatnly WR remains high during transion is extremely important for the
//safe operation of peripherals not to write to them.

#define MAKE_WR_PIN_OUTPUT() SET_SRAM_WR_PIN_DD_OUT()			// sbi(DDRG, 0)
#define MAKE_RD_PIN_OUTPUT() SET_SRAM_RD_PIN_DD_OUT()				// sbi(DDRG, 1)                 
#define MAKE_WR_PIN_INPUT() SET_SRAM_WR_PIN_DD_IN()				// cbi(DDRG, 0)                  //this pins should be alwas input not to fight with CPLD
#define MAKE_RD_PIN_INPUT() SET_SRAM_RD_PIN_DD_IN()				// cbi(DDRG, 1)                  //this pins should be alwas input not to fight with CPLD

//  SRW11   SRW10
//    0       0       no wait states
//    0       1       wait one cycle during R/W strobe
//    1       0       wait two cycles during R/W strobe
//    1       1       wait two cycles during R/W strobe, wait one cycle before driving new address
#define SET_SRAM_WAIT_STATES() {\
            sbi(XMCRA,SRW11);   \
            sbi(MCUCR,SRW10);   \
}      

//NOTE: Bits 5..2 of the MCUCR register control the sleep mode. 
#define MAKE_SRAM_60K() XMCRB=((0<<XMM2) | (0<<XMM1) | (0<<XMM0))                //make memory space 60KB. 
//#define SET_RAM_CONTROL_LINES_IN_SAFE_MODE() cbi(DDRG, 0); cbi(DDRG, 1); sbi(PORTG, 0); sbi(PORTG, 1); outp((1<<XMBK),XMCRB) //make RD,WR input pins and enable the bus keeper 
// Pull-ups keep the memories off (*** may not be required ***)
#define SET_RAM_CONTROL_LINES_IN_SAFE_MODE() {\
                      cbi(DDRG, 0);           \
                      cbi(DDRG, 1);           \
                      sbi(PORTG,0);           \
                     sbi(PORTG, 1);           \
                   cbi(XMCRB,XMBK);           \
} 
 //make RD,WR input pins and disable the bus keeper 
#define SET_RAM_CONTROL_LINES_ZERO(){	\
					sbi(DDRG, 0);		\
					sbi(DDRG, 1);		\
					cbi(PORTG, 0);		\
					cbi(PORTG, 1);		\
					cbi(XMCRB,XMBK);	\
}					//make RD,WR output-low pins and disable the bus keeper 
#define SET_RAM_CONTROL_LINES_ONE(){	\
					sbi(DDRG, 0);		\
					sbi(DDRG, 1);		\
					sbi(PORTG, 0);		\
					sbi(PORTG, 1);		\
					cbi(XMCRB,XMBK); 	\
}
//make RD,WR output-low pins and disable the bus keeper 
#define SET_RAM_LINES_PORT_MODE() cbi(MCUCR,SRE)           //behave like ordinary registers
#define SET_RAM_LINES_EXTERNAL_SRAM_MODE() sbi(MCUCR,SRE)  //Secondry external SRAM 
#define MAKE_PORTA_OUTPUT() DDRA=0xff
#define MAKE_PORTA_INPUT() DDRA=0x00
// Pull-ups are required on PORTA because the address latch is on the same power
// supply as the uP. If the PORTA lines float, the inputs of the latch will
// draw excessive current. The address latch should be attached to the memory
// system power supply (BVcc) in future revisions of Cyclops in order to avoid
// wasting power. 
//#define MAKE_PORTA_INPUT_PULL_UP() outp(0x00,DDRA);outp(0xFF,PORTA)
#define MAKE_PORTA_INPUT_PULL_UP(){		\
					DDRA=0x00;		\
					PORTA=0xFF;		\
}					
#define MAKE_PORTA_ZERO(){				\
					PORTA=0x00; 		\
					DDRA=0xFF;		\
}
#define MAKE_PORTC_OUTPUT() DDRC=0xff
#define MAKE_PORTC_INPUT() DDRC=0x00
// Pull-ups are NOT required on PORTC because the memory system is powered down.
// Pull-ups waste power.
#define MAKE_PORTC_INPUT_NOT_PULL_UP(){	\
					DDRC=0x00;		\
					PORTC=0x00;		\
}
#define MAKE_PORTC_INPUT_PULL_UP(){		\
					DDRC=0x00;		\
					PORTC=0xFF;		\
}
#define MAKE_PORTC_ZERO(){				\
					PORTC=0x00;		\
					DDRC=0xFF;		\
}

// dives address latch
#define SET_ADDRESS_LATCH(){			\
					sbi(PORTG,2);		\
					sbi(DDRG,2);		\
}
#define CLEAR_ADDRESS_LATCH(){			\
					cbi(PORTG,2);		\
					sbi(DDRG,2);		\
}

// drives _OE input of address latch
#define MAKE_ALE_OE_OUTPUT() SET_ADDRESS_LATCH_OUTPUT_EN_DD_OUT()		// sbi(DDRE,2)
#define ADDRESS_LATCH_OUTPUT_ENABLE() CLR_ADDRESS_LATCH_OUTPUT_EN()	  	// cbi(PORTE,2)
#define ADDRESS_LATCH_OUTPUT_DISABLE() SET_ADDRESS_LATCH_OUTPUT_EN()		// sbi(PORTE,2)

//*******************cpld Voltage Regulator Functionality***************
#define MAKE_VOLTAGE_REGULATOR_PIN_OUTPUT() SET_MEM_CPLD_EN_DD_OUT()	// sbi(DDRB, 7)
#define TURN_VOLTAGE_REGULATOR_ON() CLR_MEM_CPLD_EN()				// cbi(PORTB, 7)  //by making the pin low we turn on the P-Channel Mosfet
#define TURN_VOLTAGE_REGULATOR_OFF() SET_MEM_CPLD_EN()				// sbi(PORTB, 7)  //by making the pin high we turn off the P-Channel Mosfet


extern void cpld_init_IO();

extern int8_t cpldInit();
/**
* Set the mode of the cpld.
*
* @cpldC is should be filled by the application for proper operation
*  and passed to this cpldControl component.
*
* Note that the event cpldDone will be signaled some time in future
* if command is successful.
*
* @return SUCCESS if cpld is free, FAIL id cpld is already busy
*/
extern int8_t setCpldMode(cpldCommand_t *cpldC);

/**
* force cpld to go to standby mode
* NOTE: not recommended except in start time.
*
* @return SUCCESS
*/
//
extern int8_t stdByCpld();  

/**
* cpld respone to for the setCpldMode operations that have result later.
*
* @ cpld is the address of originating command.
* Application might check the pointer address to make sure it was the
* result of its calling the component.
*
* @return SUCCESS
*/
//event result_t setCpldModeDone(cpldCommand_t *myCpld);

