
/**
 * @brief    header file for timer
 * @author   Simon Han
 * @brief    Pre-allocated safe timer blocks
 * @author   Ram Kumar
 */

/**
 * @note Modules can pre-allocate timers by specifying the num_timers field
 * in the module header. There is an upper limit on the maximum number of 
 * timers that can be pre-allocated by a module. The limit is defined in the
 * timer_conf.h file in the /processor directory.
 */

#ifndef _TIMER0_H
#define _TIMER0_H

#include <hardware_types.h>
#include <pid.h>
#include <sos_list.h>
#include <timer_conf.h>
#include <sos_module_types.h>
#include <message_types.h>

enum 
  {
    TIMER_REPEAT        = 0, //!< high priority, periodic  
    TIMER_ONE_SHOT      = 1, //!< high priority, one shot
    SLOW_TIMER_REPEAT   = 2, //!< low priority, periodic
    SLOW_TIMER_ONE_SHOT = 3, //!< low priority, one shot
  };




#define SLOW_TIMER_MASK                  0x02
#define ONE_SHOT_TIMER_MASK              0x01


enum
  {
    TIMER_PRE_ALLOCATED = 0x02, //! Indicate Timer Block is Pre-allocated  
  };

/**
 * @brief Flag helpers
 */
#define flag_timer_pre_allocated(tt)   ((tt->flag) & TIMER_PRE_ALLOCATED)


/**
 * \brief Payload of the Timer Timeout Message
 */
// Ram - Mirror the MsgParam structure with a more descriptive field name
typedef struct {
  uint8_t tid;  //!< Timer TID
  uint16_t pad; 
} PACK_STRUCT
sos_timeout_t;

/**
 * \struct sos_timer_t
 * \brief Kernel data structure for the timer (size is 16 bytes) 
 */
typedef struct {
  list_t    list;          //!< list 
  uint8_t   type;          //!< timer type
  sos_pid_t pid;           //!< module id of the timer requester
  uint8_t   tid;           //!< timer instance id
  int32_t   ticks;         //!< clock ticks for a repeat timer
  int32_t   delta;         //!< current delta value 
  uint8_t   flag;          //!< Timer block status flags
} sos_timer_t;


/**
 * Get timer tid from Message
 */
static inline uint8_t timer_get_tid( Message *msg )
{
	MsgParam* params = (MsgParam*)(msg->data);

	return params->byte;
}

/**
 * Application timer API
 */
#ifndef _MODULE_
#include <sos_types.h>	



/**
 * @brief Initialize a timer block. 
 * @param pid Module Identity
 * @param tid Timer Instance Id, only needs to be unique to the module
 * @param type Type can be {TIMER_REPEAT, TIMER_ONE_SHOT, SLOW_TIMER_REPEAT, SLOW_TIMER_ONE_SHOT
 * @return SOS_OK if a timer block is pre-allocated or already initialized or dynamically allocated
 * @return -ENOMEM if the system is unable to dynamically allocate memory
 * @return -EEXIST if the same timer is already running
 */
extern int8_t ker_timer_init(sos_pid_t pid, uint8_t tid, uint8_t type);


/**
 * @brief Start a new timer
 * @param pid module id
 * @param tid timer instance id, only need to be unique in the module
 * @param interval binary interval
 * @return SOS_OK if a timer was already initialized
 * @return -EINVAL if a timer is already running or it is not initialized
 * @return -EPERM if the timer interval is less than 5
 * @note binary interval uses following conversion
 * 1024 ticks == 1000 milliseconds
 *  512 ticks ==  500 milliseconds
 *  256 ticks ==  250 milliseconds
 *  128 ticks ==  125 milliseconds
 *
 * The interval cannot be less than 5
 */
extern int8_t ker_timer_start(sos_pid_t pid, uint8_t tid, int32_t interval);

/**
 * @brief Restart a timer
 * @param pid Modue Id
 * @param tid Timer instance id
 * @param interval Binary Interval
 * @return SOS_OK upon success
 * @return -EINVAL if the timer is neither running nor initialized
 * @return -EPERM if the timer interval is less than 5
 * @note Refer to the ker_timer_start API for discussion on the interval
 * ker_timer_restart will:
 * 1. Either stop a running timer and restart it with the new interval
 * 2. Start an initialized timer
 * It will return an error if a timer is not initialized.
 */
extern int8_t ker_timer_restart(sos_pid_t pid, uint8_t tid, int32_t interval);

/**
 * @brief Stop a timer
 * @param pid module id
 * @param tid timer instance id to be stopped
 * @return SOS_OK upon success
 * @return -EINVAL if the timer is not running
 * @note This call will only stop the timer and it will NOT de-allocate its resources.
 * Use the ker_timer_release to deep free timer memory when a timer is no longer needed.
 */
extern int8_t ker_timer_stop(sos_pid_t pid, uint8_t tid);


/**
 * @brief Deep free of a running or initialized timer
 * @param pid Module Id
 * @param tid Timer instance ID to be released
 * @return SOS_OK Upon success
 * @return -EINVAL If no such timer exists
 * @note This call does not free UNINITIALIZED PRE-ALLOCATED timers
 */
extern int8_t ker_timer_release(sos_pid_t pid, uint8_t tid);


//------------------------------------------------------------------------
// KERNEL ACCESSIBLE FUNCTIONS
//------------------------------------------------------------------------

/**
 * @brief Init function
 */
extern void timer_init(void);


/**
 * @brief Initialize a permanent timer for the kernel. 
 * @note
 * 1. The space for the timer is statically allocated.
 * 2. This API is accessible only to the statically compiled modules.
 * 3. It is used for reducing the memory consumption.
 */
extern int8_t ker_permanent_timer_init(sos_timer_t* tt, sos_pid_t pid, uint8_t tid, uint8_t type);




/**
 * @brief Remove timers of a particular pid
 */
extern int8_t timer_remove_all(sos_pid_t pid);


/**
 * @brief Pre allocate requested timers at module load time
 */
extern int8_t timer_preallocate(sos_pid_t pid, uint8_t num_timers);


/**
 * @brief Micro-reboot the timer service for the given module
 */
#ifdef FAULT_TOLERANT_SOS
extern int8_t timer_micro_reboot(sos_module_t *handle);
#endif

/* *************************************************
 * Real Time Timer Support for kernel drivers
 * *************************************************/

/**
 * function callback for real time clock
 * the callback is called in the hardware interrupt.  
 * Therefore, if there are any data sharing in this callback and other 
 * functions, one has to use CRITICAL_SECTION macro to protect racing 
 * condition.
 */
typedef void (*timer_callback_t)(void);

/**
 * Start a realtime timer
 *
 * @param uint16_t value initial value of timer ticks
 * @param uint16_t interval reload value of time ticks
 * @param timer_callback_t f timer callback function
 *
 * timer_realtime_start starts a timer with value in 1/1024 seconds.  
 * When value amount of time is passed, f is called.  
 * If interval is not zero, timer is reloaded with interval.
 */
int8_t timer_realtime_start(uint16_t value, uint16_t interval, timer_callback_t f);

/**
 * Stop a realtime timer with callback
 */ 
int8_t timer_realtime_stop(timer_callback_t f);


#endif /* _MODULE_ */

#endif /* _TIMER0_H */
