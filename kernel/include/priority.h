#ifndef _PRIORITY_H_
#define _PRIORITY_H_

#include <sos_types.h>
#include <pid.h>

//----------------------------------------------------------------------------------
// Typedefs
//
typedef uint8_t pri_t;       //!< priority type

enum {
  DISABLED = 0,
  ENABLED,
};

//----------------------------------------------------------------------------------
// Global to maintain if preemption is enabled or disabled
//
uint8_t preemption_status;

//----------------------------------------------------------------------------------
// Macros
//

/**
 * The following group of preemption macros can
 * only be used for SIGNALS. Please notice that 
 * SIGNALS and INTERRUPTS are different. SIGNALS
 * disable interrupts
 */

#define HAS_PREEMPTION_SECTION register uint8_t _prem_prev_

#define DISABLE_PREEMPTION()                    \
  do {                                          \
    _prem_prev_ = preemption_status;		\
    preemption_status = DISABLED;} while(0)

#define ENABLE_PREEMPTION()			\
  do {                                          \
    preemption_status = _prem_prev_;		\
    sched_msg_alloc(NULL);} while(0)

/**
 * The following group of preemption macros are
 * more general and can be used for INTERRUPTS 
 * and in modules
 */

#define HAS_ATOMIC_PREEMPTION_SECTION           \
  do {                                          \
    HAS_CRITICAL_SECTION;			\
    register uint8_t _prem_prev_;} while(0)

#define ATOMIC_DISABLE_PREEMPTION()             \
  do {                                          \
    ENTER_CRITICAL_SECTION();			\
    _prem_prev_ = preemption_status;		\
    preemption_status = DISABLED;		\
    LEAVE_CRITICAL_SECTION();} while(0)

#define ATOMIC_ENABLE_PREEMPTION()		\
  do {                                          \
    ENTER_CRITICAL_SECTION();			\
    preemption_status = _prem_prev_;		\
    LEAVE_CRITICAL_SECTION();			\
    sched_msg_alloc(NULL);} while(0)

#define GET_PREEMPTION_STATUS() preemption_status

// Returns the priority of the module with pid id
pri_t get_module_priority(sos_pid_t id);
// Set the given priority for the module with pid id
void set_module_priority(sos_pid_t id, pri_t priority);

#endif // _PRIORITY_H_
