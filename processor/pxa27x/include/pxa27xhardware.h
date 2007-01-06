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
 * @author Philip Buonadonna
 */


/*
 * HOW DO WE ASSIGN PINS?
 *
 *
 */

#ifndef PXA27X_HARDWARE_H
#define PXA27X_HARDWARE_H

//#include <hardware.h>
//#include "arm_defs.h"
#include "pxa27x_registers.h"

#define SOS_ASSIGN_PIN(name, port, regbit) \
static inline void SOS_SET_##name##_PIN() {_GPSR(regbit) |= _GPIO_bit(regbit);} \
static inline void SOS_CLR_##name##_PIN() {_GPCR(regbit) |= _GPIO_bit(regbit);} \
static inline void SOS_TOGGLE_##name##_PIN() {if (_GPLR(regbit) & _GPIO_bit(regbit)) 			\
												_GPCR(regbit) |= _GPIO_bit(regbit);				\
												else _GPSR(regbit) |= _GPIO_bit(regbit);} 		\
static inline char SOS_READ_##name##_PIN() {return ((_GPLR(regbit) & _GPIO_bit(regbit)) != 0);} \
static inline void SOS_MAKE_##name##_OUTPUT() {_GPIO_setaltfn(regbit,0);_GPDR(regbit) |= _GPIO_bit(regbit);} \
static inline void SOS_MAKE_##name##_INPUT() {_GPIO_setaltfn(regbit,0);_GPDR(regbit) &= ~(_GPIO_bit(regbit));}

#define SOS_ASSIGN_OUTPUT_ONLY_PIN(name, port, regbit) \
static inline void SOS_SET_##name##_PIN() {_GPSR(regbit) |= _GPIO_bit(regbit);} \
static inline void SOS_CLR_##name##_PIN() {_GPCR(regbit) |= _GPIO_bit(regbit);} \
static inline void SOS_MAKE_##name##_OUTPUT() {_GPDR(regbit) |= _GPIO_bit(regbit);}

// GPIO Interrupt Defines
#define SOS_RISING_EDGE (1)
#define SOS_FALLING_EDGE (2)
#define SOS_BOTH_EDGE (3)

typedef uint32_t __local_atomic_t;

void TOSH_wait();
void TOSH_sleep();
inline void TOSH_uwait(uint16_t usec);
inline uint32_t _pxa27x_clzui(uint32_t i);
inline __local_atomic_t local_atomic_start(void);
inline void local_atomic_end(__local_atomic_t oldState);
inline void local_irq_enable();
inline void local_irq_disable();
//inline void __atomic_sleep();

#endif //PXA27X_HARDWARE_H
