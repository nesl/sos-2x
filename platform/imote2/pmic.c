/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License
 *
 *  Copyright (c) 2002 Intel Corporation
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *      Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTEL OR ITS
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

/*
 *
 * Authors:  Lama Nachman, Robert Adler
 */

#include <hardware.h>

#define START_RADIO_LDO 1
#define START_SENSOR_BOARD_LDO 1

/*
 * VCC_MEM is connected to LDO by default,
 * If the imote2 board has R49 on and R40 off then ENABLE_BUCK2 can be set t0 0
 * This is assumed to be the default setting.
 * If the imote2 board has R49 off and R40 on, you need to set ENABLE_BUCK2
 * to 1 when making the app.
 */
//#ifndef ENABLE_BUCK2
//#define ENABLE_BUCK2 1
//#endif

//uses {
//  interface PXA27XInterrupt as PI2CInterrupt;
//  interface PXA27XGPIOInt as PMICInterrupt;
//  interface Timer as chargeMonitorTimer;
//  interface Reset;
//}
#define result_t uint8_t
enum {
	SUCCESS = 0,
	FAIL = 1
};

#include "pmic.h"

uint8_t pmic_init();
result_t readPMIC(uint8_t address, uint8_t *value, uint8_t numBytes);
result_t writePMIC(uint8_t address, uint8_t value);
void startLDOs();
result_t pmic_start();
result_t pmic_stop();
void PI2CInterrupt_fired();
void PMICInterrupt_fired();
result_t PMIC_setCoreVoltage(uint8_t trimValue);
result_t PMIC_shutDownLDOs();

bool gotReset;

uint8_t pmic_init()
{
	CKEN |= CKEN15_PMI2C;
	PCFR |= PCFR_PI2C_EN;
	PICR = ICR_IUE | ICR_SCLE;
	//atomic{
	  gotReset=FALSE;
	//}
	PXA27XIrq_allocate(PPID_PWR_I2C,PI2CInterrupt_fired);
	pmic_start();
	return 0;
}

result_t readPMIC(uint8_t address, uint8_t *value, uint8_t numBytes)
{
	//send the PMIC the address that we want to read
	if(numBytes > 0) {
		PIDBR = PMIC_SLAVE_ADDR<<1;
		PICR |= ICR_START;
		PICR |= ICR_TB;
		while(PICR & ICR_TB);

		//actually send the address terminated with a STOP
		PIDBR = address;
		PICR &= ~ICR_START;
		PICR |= ICR_STOP;
		PICR |= ICR_TB;
		while(PICR & ICR_TB);
		PICR &= ~ICR_STOP;

		//actually request the read of the data
		PIDBR = PMIC_SLAVE_ADDR<<1 | 1;
		PICR |= ICR_START;
		PICR |= ICR_TB;
		while(PICR & ICR_TB);
		PICR &= ~ICR_START;

		//using Page Read Mode
		while (numBytes > 1) {
			PICR |= ICR_TB;
			while(PICR & ICR_TB);
			*value = PIDBR;
			value++;
			numBytes--;
		}
		PICR |= ICR_STOP;
		PICR |= ICR_ACKNAK;
		PICR |= ICR_TB;
		while(PICR & ICR_TB);
		*value = PIDBR;
		PICR &= ~ICR_STOP;
		PICR &= ~ICR_ACKNAK;

		return SUCCESS;
	}
    else {
		return FAIL;
	}
}

result_t writePMIC(uint8_t address, uint8_t value)
{
	PIDBR = PMIC_SLAVE_ADDR<<1;
	PICR |= ICR_START;
	PICR |= ICR_TB;
	while(PICR & ICR_TB);

	PIDBR = address;
	PICR &= ~ICR_START;
	PICR |= ICR_TB;
	while(PICR & ICR_TB);

	PIDBR = value;
	PICR |= ICR_STOP;
	PICR |= ICR_TB;
	while(PICR & ICR_TB);
	PICR &= ~ICR_STOP;

	return SUCCESS;
}

void startLDOs()
{
	uint8_t oldVal, newVal;
//#if 0
//#if START_SENSOR_BOARD_LDO
	// TODO : Need to move out of here to sensor board functions
	readPMIC(PMIC_A_REG_CONTROL_1, &oldVal, 1);
	newVal = oldVal | ARC1_LDO10_EN | ARC1_LDO11_EN;	// sensor board
	writePMIC(PMIC_A_REG_CONTROL_1, newVal);

	readPMIC(PMIC_B_REG_CONTROL_2, &oldVal, 1);
	newVal = oldVal | BRC2_LDO10_EN | BRC2_LDO11_EN;
	writePMIC(PMIC_B_REG_CONTROL_2, newVal);
//#endif
//#endif
// ENABLE BB ????
	readPMIC(PMIC_B_REG_CONTROL_1, &oldVal, 1);
	newVal = oldVal | BRC1_LDO3_EN;
	writePMIC(PMIC_B_REG_CONTROL_1, newVal);

	readPMIC(PMIC_B_REG_CONTROL_2, &oldVal, 1);
	newVal = oldVal | BRC2_LDO12_EN;
	writePMIC(PMIC_B_REG_CONTROL_2, newVal);

//#if 0
//#if START_RADIO_LDO
	// TODO : Move to radio start
	readPMIC(PMIC_B_REG_CONTROL_1, &oldVal, 1);
	newVal = oldVal | BRC1_LDO5_EN;
	writePMIC(PMIC_B_REG_CONTROL_1, newVal);
//#endif

//#if (!ENABLE_BUCK2)  // Disable BUCK2 if VCC_MEM is not configured to use BUCK2
//	readPMIC(PMIC_B_REG_CONTROL_1, &oldVal, 1);
//	newVal = oldVal & ~BRC1_BUCK_EN;
//	writePMIC(PMIC_B_REG_CONTROL_1, newVal);
//#endif
//#endif

#if 0
	// Configure above LDOs, Radio and sensor board LDOs to turn off in sleep
	// TODO : Sleep setting doesn't work
	temp = BSC1_LDO1(1) | BSC1_LDO2(1) | BSC1_LDO3(1) | BSC1_LDO4(1);
	writePMIC(PMIC_B_SLEEP_CONTROL_1, temp);
	temp = BSC2_LDO5(1) | BSC2_LDO7(1) | BSC2_LDO8(1) | BSC2_LDO9(1);
	writePMIC(PMIC_B_SLEEP_CONTROL_2, temp);
	temp = BSC3_LDO12(1);
	writePMIC(PMIC_B_SLEEP_CONTROL_3, temp);
#endif
}

result_t pmic_start()
{
	//init unit
	//uint8_t val[3];
	uint8_t val[8];
	//uint8_t i;

	PXA27XIrq_enable(PPID_PWR_I2C);
	//irq is apparently active low...however trigger on both for now
	//call PMICInterrupt.enable(TOSH_FALLING_EDGE);
	PXA27XGPIOInt_enable(1,SOS_FALLING_EDGE,PMICInterrupt_fired);

	/*
	 * Reset the watchdog, switch it to an interrupt, so we can disable it
	 * Ignore SLEEP_N pin, enable H/W reset via button
	 */
	writePMIC(PMIC_SYS_CONTROL_A,
	          SCA_RESET_WDOG | SCA_WDOG_ACTION | SCA_HWRES_EN);

	//writePMIC(PMIC_SYS_CONTROL_A, 5);
	//for(i=0; i<8; i++) val[i] = 0;
	//readPMIC(PMIC_SYS_CONTROL_A,val,8);

	// Disable all interrupts from PMIC except for ONKEY button
	writePMIC(PMIC_IRQ_MASK_A, ~IMA_ONKEY_N);
	writePMIC(PMIC_IRQ_MASK_B, 0xFF);
	writePMIC(PMIC_IRQ_MASK_C, 0xFF);

	//read out the EVENT registers so that we can receive interrupts
	readPMIC(PMIC_EVENTS, val, 3);

	// Set default core voltage to 0.85 V
	//call PMIC.setCoreVoltage(B2R1_TRIM_P85_V);
	PMIC_setCoreVoltage(B2R1_TRIM_P95_V);
	//PMIC_setCoreVoltage(B2R1_TRIM_P85_V);

	startLDOs();
	return SUCCESS;
}

result_t pmic_stop()
{
	PXA27XIrq_disable(PPID_PWR_I2C);
	//call PMICInterrupt.disable();
	PXA27XGPIOInt_disable(1);
	CKEN &= ~CKEN15_PMI2C;
	PICR = 0;

	return SUCCESS;
}

void PI2CInterrupt_fired()
{
	uint32_t status, update=0;
	status = PISR;
	if(status & ISR_ITE){
		update |= ISR_ITE;
		//trace(DBG_USR1,"sent data");
	}

	if(status & ISR_BED){
		update |= ISR_BED;
		//trace(DBG_USR1,"bus error");
	}
	PISR = update;
}

//void resetTask()
//{
//	call Reset.reset();
//}

void PMICInterrupt_fired()
{
	uint8_t events[3];
	bool localGotReset;

	//call PMICInterrupt.clear();
	PXA27XGPIOInt_clear(1);

	readPMIC(PMIC_EVENTS, events, 3);

	if(events[EVENTS_A_OFFSET] & EA_ONKEY_N){
		//atomic{
		localGotReset = gotReset;
		//}
		if(localGotReset==TRUE){
			//eliminate error since Reset.reset is not declared as async
			//post resetTask();
			// ?????????
 		}
		else{
			//atomic{
			gotReset=TRUE;
			//}
		}
	}
	//else{
	//	trace(DBG_USR1,"PMIC EVENTs =%#x %#x %#x\r\n",events[0], events[1], events[2]);
	//}
}

/*
 * The Buck2 controls the core voltage, set to appropriate trim value
 */
result_t PMIC_setCoreVoltage(uint8_t trimValue)
{
	writePMIC(PMIC_BUCK2_REG1, (trimValue & B2R1_TRIM_MASK) | B2R1_GO);
	return SUCCESS;
}

result_t PMIC_shutDownLDOs()
{
	uint8_t oldVal, newVal;
	/*
	 * Shut down all LDOs that are not controlled by the sleep mode
	 * Note, we assume here the LDO10 & LDO11 (sensor board) will be off
	 * Should be moved to sensor board control
	 */

	// LDO1, LDO4, LDO6, LDO7, LDO8, LDO9, LDO10, LDO 11, LDO13, LDO14

	readPMIC(PMIC_A_REG_CONTROL_1, &oldVal, 1);
	newVal = oldVal & ~ARC1_LDO13_EN & ~ARC1_LDO14_EN;
	newVal = newVal & ~ARC1_LDO10_EN & ~ARC1_LDO11_EN;	// sensor board
	writePMIC(PMIC_A_REG_CONTROL_1, newVal);

	readPMIC(PMIC_B_REG_CONTROL_1, &oldVal, 1);
	newVal = oldVal & ~BRC1_LDO1_EN & ~BRC1_LDO4_EN & ~BRC1_LDO5_EN &
				~BRC1_LDO6_EN & ~BRC1_LDO7_EN;
	writePMIC(PMIC_B_REG_CONTROL_1, newVal);

	readPMIC(PMIC_B_REG_CONTROL_2, &oldVal, 1);
	newVal = oldVal & ~BRC2_LDO8_EN & ~BRC2_LDO9_EN & ~BRC2_LDO10_EN &
				~BRC2_LDO11_EN & ~BRC2_LDO14_EN & ~BRC2_SIMCP_EN;
	writePMIC(PMIC_B_REG_CONTROL_2, newVal);

	return SUCCESS;
}

//uint8_t
result_t getPMICADCVal(uint8_t channel, uint8_t *val)
{
	uint8_t oldval;
	//result_t rval;

	//read out the old value so that we can reset at the end
	//rval=
	readPMIC(PMIC_ADC_MAN_CONTROL, &oldval,1);
	//rcombine(rval,
	writePMIC(PMIC_ADC_MAN_CONTROL, PMIC_AMC_ADCMUX(channel) |
				PMIC_AMC_MAN_CONV | PMIC_AMC_LDO_INT_Enable);//);
	//rcombine(rval,
	readPMIC(PMIC_MAN_RES,val,1);//);
	//reset to old state
	//rcombine(rval,
	writePMIC(PMIC_ADC_MAN_CONTROL, oldval);//);
	return 0;//*val;//rval;
}

result_t PMIC_getBatteryVoltage(uint8_t *val)
{
	//for now, let's use the manual conversion mode
	return getPMICADCVal(0, val);
}

result_t PMIC_chargingStatus(uint8_t *vBat, uint8_t *vChg, uint8_t *iChg)
{
	getPMICADCVal(0, vBat);
	getPMICADCVal(2, vChg);
	getPMICADCVal(1, iChg);
	return SUCCESS;
}

result_t PMIC_enableAutoCharging(bool enable)
{
	return SUCCESS;
}

result_t PMIC_enableManualCharging(bool enable)
{
	//just turn on or off the LED for now!!
	uint8_t val;

	if(enable){
		//want to turn on the charger
		getPMICADCVal(2, &val);
		//if charger is present due some stuff...75 should be 4.65V or so
		if(val > 75 ) {
			//trace(DBG_USR1,"Charger Voltage is %.3fV...enabling charger...\r\n", ((val*6) * .01035));
			//write the total timeout to be 8 hours
			writePMIC(PMIC_TCTR_CONTROL,8);
			//enable the charger at 100mA and 4.35V
			writePMIC(PMIC_CHARGE_CONTROL,PMIC_CC_CHARGE_ENABLE | PMIC_CC_ISET(1) | PMIC_CC_VSET(7));
			//turn on the LED
			writePMIC(PMIC_LED1_CONTROL,0x80);
			//start a timer to monitor our progress every 5 minutes!
//			call chargeMonitorTimer.start(TIMER_REPEAT,300000);
		}
		else{
			//trace(DBG_USR1,"Charger Voltage is %.3fV...charger not enabled\r\n", ((val*6) * .01035));
		}
	}
	else{
		//turn off the charger and the LED
		//call
		PMIC_getBatteryVoltage(&val);
		//trace(DBG_USR1,"Disabling Charger...Battery Voltage is %.3fV\r\n", (val * .01035) + 2.65);
		//disable everything that we enabled
		writePMIC(PMIC_TCTR_CONTROL,0);
		writePMIC(PMIC_CHARGE_CONTROL,0);
		writePMIC(PMIC_LED1_CONTROL,0x00);
	}
	return SUCCESS;
}

result_t chargeMonitorTimer_fired()
{
	uint8_t val;
	//call
	PMIC_getBatteryVoltage(&val);
	//stop when vBat>4V
	if(val>130){
		PMIC_enableManualCharging(FALSE);
//		chargeMonitorTimer_stop();
	}
	return SUCCESS;
}

