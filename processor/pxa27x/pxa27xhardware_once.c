#include <hardware.h>
#include "arm_defs.h"

void TOSH_wait()
{
  asm volatile("nop");
  asm volatile("nop");
}

void TOSH_sleep()
{
#if 0
  // Place PXA into idle
  asm volatile (
		"mcr p14,0,%0,c7,c0,0"
		:
		: "r" (1)
		);
#endif
}

/**
 * (Busy) wait <code>usec</code> microseconds
 */
inline void TOSH_uwait(uint16_t usec)
{
  uint32_t start,mark = usec;

  //in order to avoid having to reset OSCR0, we need to look at time differences

  start = OSCR0;
  mark <<= 2;
  mark *= 13;
  mark >>= 2;

  //OSCR0-start should work correctly due to nice properties of underflow
  while ( (OSCR0 - start) < mark);
}

inline uint32_t _pxa27x_clzui(uint32_t i) {
  uint32_t count;
  asm volatile ("clz %0,%1"
		: "=r" (count)
		: "r" (i)
		);
  return count;
}

//typedef uint32_t __nesc_atomic_t;

//NOTE...at the moment, these functions will ONLY disable the IRQ...FIQ is left alone
inline __local_atomic_t local_atomic_start(void)
{
  uint32_t result = 0;
  uint32_t temp = 0;

  asm volatile (
		"mrs %0,CPSR\n\t"
		"orr %1,%2,%4\n\t"
		"msr CPSR_cf,%3"
		: "=r" (result) , "=r" (temp)
		: "0" (result) , "1" (temp) , "i" (ARM_CPSR_INT_MASK)
		);
  return result;
}

inline void local_atomic_end(__local_atomic_t oldState)
{
  uint32_t  statusReg = 0;
  //make sure that we only mess with the INT bit
  oldState &= ARM_CPSR_INT_MASK;
  asm volatile (
		"mrs %0,CPSR\n\t"
		"bic %0, %1, %2\n\t"
		"orr %0, %1, %3\n\t"
		"msr CPSR_c, %1"
		: "=r" (statusReg)
		: "0" (statusReg),"i" (ARM_CPSR_INT_MASK), "r" (oldState)
		);

  return;
}

inline void local_irq_enable() {
	unsigned long temp;
	__asm__ __volatile__(
	"mrs	%0, cpsr		@ local_irq_enable\n\t"
	"bic	%0, %0, #0xc0\n\t"
	"msr	cpsr_c, %0"
	: "=r" (temp)
	:
	: "memory", "cc");
	return;
}

inline void local_irq_disable() {
	unsigned long temp;
	__asm__ __volatile__(
	"mrs	%0, cpsr		@ local_irq_disable\n\t"
	"orr	%0, %0, #0xc0\n\t"
	"msr	cpsr_c, %0"
	: "=r" (temp)
	:
	: "memory", "cc");
	return;
}

inline void __atomic_sleep()
{
  /*
   * Atomically enable interrupts and sleep ,
   * LN : FOR NOW SLEEP IS DISABLED will be adding this functionality shortly
   */
  local_irq_enable();
  return;
}

