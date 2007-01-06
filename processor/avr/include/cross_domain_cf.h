/**
 * \file cross_domain_cf.h
 * \brief Cross domain control flow checks and routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _CROSS_DOMAIN_CF_H_
#define _CROSS_DOMAIN_CF_H_

/**
 * \addtogroup runtimechecker
 * Sequence of optimized assembly routines for performing run-time checks
 * @{
 */

/**
 * \brief Identity of current active domain
 */
extern uint8_t curr_dom_id;


/**
 * \brief Current stack bound
 */
extern uint16_t domain_stack_bound;


/**
 * \brief Cross domain call from kernel into the module while invoking message handler
 */
int8_t ker_cross_domain_call_mod_handler(void* state, Message* m, msg_handler_t handler);

/**
 * \brief Assembly routine to save correct return address to SafeStack upon RCALL instructin
 */
void ker_save_ret_addr();

/**
 * \brief Assembly routine to restore correct return address on <STRONG>RET</STRONG> instruction
 */
void ker_restore_ret_addr();

/**
 * \brief Assembly routine to check the control flow on <STRONG>ICALL</STRONG> instruction
 */
void ker_icall_check();

/* @} */

#endif//_CROSS_DOMAIN_CALL_KERNEL_H_
