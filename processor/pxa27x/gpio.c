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
 * @author Phil Buonadonna
 *
 */

#include "hardware.h"

#define result_t int

//#define bool int
//bool gfInitialized = FALSE;

void GPIOIrq0_fired();
void GPIOIrq1_fired();
void GPIOIrq_fired();

void (*gpio_callback[121])(void);

result_t PXA27XGPIOInt_init()
{
	//bool isInited;
    //atomic {
	//isInited = gfInitialized;
	//gfInitialized = TRUE;
    //}
	//if (!isInited) {
		PXA27XIrq_allocate(PPID_GPIO_0,GPIOIrq0_fired);
		PXA27XIrq_allocate(PPID_GPIO_1,GPIOIrq1_fired);
		PXA27XIrq_allocate(PPID_GPIO_X,GPIOIrq_fired);
	//}
	PXA27XGPIOInt_start();
    return 0;	//SUCCESS;
}

result_t PXA27XGPIOInt_start()
{
	PXA27XIrq_enable(PPID_GPIO_0);
	PXA27XIrq_enable(PPID_GPIO_1);
	PXA27XIrq_enable(PPID_GPIO_X);
	return 0;	//SUCCESS;
}

result_t PXA27XGPIOInt_stop()
{
	return 0;	//SUCCESS;
}

void PXA27XGPIOInt_enable(uint8_t pin, uint8_t mode, void(*callback)(void))
{
	if (pin < 121) {
		switch (mode) {
			case SOS_RISING_EDGE:
				_GRER(pin) |= _GPIO_bit(pin);
				_GFER(pin) &= ~(_GPIO_bit(pin));
				break;
			case SOS_FALLING_EDGE:
				_GRER(pin) &= ~(_GPIO_bit(pin));
				_GFER(pin) |= _GPIO_bit(pin);
				break;
			case SOS_BOTH_EDGE:
				_GRER(pin) |= _GPIO_bit(pin);
				_GFER(pin) |= _GPIO_bit(pin);
				break;
			default:
				break;
		}
		gpio_callback[pin] = callback;
	}
	return;
}

void PXA27XGPIOInt_disable(uint8_t pin)
{
	if (pin < 121) {
		_GRER(pin) &= ~(_GPIO_bit(pin));
		_GFER(pin) &= ~(_GPIO_bit(pin));
		gpio_callback[pin] = NULL;
	}
	return;
}

void PXA27XGPIOInt_clear(uint8_t pin)
{
	if (pin < 121) {
		_GEDR(pin) = _GPIO_bit(pin);
	}
	return;
}

void GPIOIrq_fired()
{
	uint32_t DetectReg;
	uint8_t pin;

	// Mask off GPIO 0 and 1 (handled by direct IRQs)
	//atomic
	DetectReg = (GEDR0 & ~((1<<1) | (1<<0)));
    while (DetectReg) {
		pin = 31 - _pxa27x_clzui(DetectReg);
		//signal PXA27XGPIOInt.fired[pin]();
		if (gpio_callback[pin])
				gpio_callback[pin]();
		DetectReg &= ~(1 << pin);
	}
    //atomic
    DetectReg = GEDR1;
    while (DetectReg) {
		pin = 31 - _pxa27x_clzui(DetectReg);
		//signal PXA27XGPIOInt.fired[(pin+32)]();
		if (gpio_callback[pin])
				gpio_callback[pin]();
		DetectReg &= ~(1 << pin);
	}
	//atomic
	DetectReg = GEDR2;
	while (DetectReg) {
		pin = 31 - _pxa27x_clzui(DetectReg);
		//signal PXA27XGPIOInt.fired[(pin+64)]();
		if (gpio_callback[pin])
				gpio_callback[pin]();
		DetectReg &= ~(1 << pin);
	}
	//atomic
	DetectReg = GEDR3;
	while (DetectReg) {
		pin = 31 - _pxa27x_clzui(DetectReg);
		//signal PXA27XGPIOInt.fired[(pin+96)]();
		if (gpio_callback[pin])
				gpio_callback[pin]();
		DetectReg &= ~(1 << pin);
	}
	return;
}

void GPIOIrq0_fired()
{
	//signal PXA27XGPIOInt.fired[0]();
	if (gpio_callback[0])
			gpio_callback[0]();
}

void GPIOIrq1_fired()
{
	//signal PXA27XGPIOInt.fired[1]();
	if (gpio_callback[1])
			gpio_callback[1]();
}
