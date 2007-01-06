
#ifndef _HARDWARE_PROC_H
#define _HARDWARE_PROC_H
#include <sos_inttypes.h>


/**
 * @brief macro definition
 */
#define NOINIT_VAR
/**
 * @brief initialize hardware
 */

extern void hardware_init(void);
extern void hardware_exit(int code);

/**
 * @brief hardware related simulation
 */
extern void hardware_sleep(void);
extern void hardware_wakeup(void);
#define atomic_hardware_sleep() sei_pc(); hardware_sleep()


/**
 * @brief interrupt helpers
 */
extern uint32_t int_enabled;
extern void sei_pc(void);
extern void cli_pc(void);
extern void hw_get_interrupt_lock( void );
extern void hw_release_interrupt_lock( void );
#define HAS_CRITICAL_SECTION       register uint32_t _prev_
#define ENTER_CRITICAL_SECTION()   _prev_ = int_enabled; cli_pc()
#define LEAVE_CRITICAL_SECTION()   if(_prev_) sei_pc()
#define ENABLE_GLOBAL_INTERRUPTS() sei_pc()
#define DISABLE_GLOBAL_INTERRUPTS() cli_pc()


/**
 * @brief watchdog
 */
#define watchdog_reset()

typedef uint32_t mod_header_ptr;

typedef uint32_t func_cb_ptr;

#define SOS_MODULE_HEADER

#define sos_get_header_address(x) ((uint32_t)&(x))

#define sos_get_header_member(header, offset) ((uint32_t)((header)+(offset)))

#define sos_read_header_byte(addr, offset) (*(uint8_t*)(addr + offset))
#define sos_read_header_word(addr, offset) (*(uint16_t*)(addr + offset))
#define sos_read_header_ptr(addr, offset)  (*(void**)(addr + offset))


#if defined(EMU_MICA2) || defined(EMU_XYZ)
#define SOS_EMU
#endif

/**
 * @brief pgm functions
 */
#ifdef EMU_MICA2
extern uint16_t pgm_read_word_far(uint32_t addr);
extern uint16_t pgm_read_word(uint32_t addr);
#endif

#ifdef EMU_XYZ
extern uint32_t pgm_read_word_far(uint32_t addr);
extern uint32_t pgm_read_word(uint32_t addr);
#endif

extern uint8_t pgm_read_byte(uint32_t addr);
/**
 * @brief bit level functions
 */
#define cbi(val, pos) ((val) &= ~(1 << pos))


/**
 * Utility functions for simulating interrupts
 */
void interrupt_init(void);

void interrupt_add_read_fd(int fd, void (*callback)(int) );

void interrupt_remove_read_fd(int fd);

int interrupt_get_elapsed_time( void );

void interrupt_set_timeout(int timeout, void (*callback)(void) );

void interrupt_add_callbacks( void (*callback)(void) );

void interrupt_loop( void );


#include <message_types.h>
/**
 * @fn      void msg_header_out(uint8_t *prefix, Message *m)
 * @brief print out message header with prefix
 * @param prefix the prefix of message header
 * @param m      Message pointer
 */
extern void msg_header_out(uint8_t *prefix, Message *m);


#endif

