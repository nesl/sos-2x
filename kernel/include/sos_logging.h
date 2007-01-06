
#ifndef __SOS_LOGGING_H__
#define __SOS_LOGGINE_H__

/**
 * Number of buffers in the logger
 * Bigger buffer uses more memory but has less messaging overhead
 */
#define SOS_LOG_NUM_BUFFERS   16

enum {
	SOS_LOG_MALLOC        = 1,
	SOS_LOG_FREE          = 2,
	SOS_LOG_CHANGE_OWN    = 3,
	SOS_LOG_POST_SHORT    = 4,
	SOS_LOG_POST_LONG     = 5,
	SOS_LOG_POST_NET      = 6,
	SOS_LOG_TIMER_START   = 7,
	SOS_LOG_TIMER_RESTART = 8,
	SOS_LOG_TIMER_STOP    = 9,
	SOS_LOG_CMEM_ALLOC    = 10,
	SOS_LOG_CMEM_FREE     = 11,
	SOS_LOG_CMEM_WRITE    = 12,
	SOS_LOG_CMEM_READ     = 13,
	SOS_LOG_HANDLE_MSG    = 14,
	SOS_LOG_HANDLE_MSG_END = 15,
};

typedef struct sos_log_t {
	sos_pid_t pid;
	uint8_t   type;
	uint16_t  val;
} sos_log_t;

#ifdef SOS_USE_LOGGING
void ker_log( uint8_t type, sos_pid_t mod_id, uint16_t val );

void ker_log_flush( void );

void ker_log_start();
#else
#define ker_log( a, b, c)
#define ker_log_flush()
#define ker_log_start()
#endif

#endif

