#ifndef _INTERRUPT_CTRL_KERNEL_H_
#define _INTERRUPT_CTRL_KERNEL_H_

#include "interrupt_ctrl.h"

extern int8_t interrupt_init();
extern int8_t ker_register_isr(sos_pid_t mod_id, sos_interrupt_t int_id, uint8_t isr_fid);
extern int8_t ker_deregister_isr(sos_pid_t mod_id, sos_interrupt_t int_id);

#endif

