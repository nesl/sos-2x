
#include <sos.h>
#include <sos_logging.h>
#include <led.h>

#ifdef SOS_USE_LOGGING
static sos_log_t *log_buffer = NULL;
static uint8_t   log_index = 0;
static bool log_started = false;
static bool log_in_logging = false;
static bool log_in_flush = false;

void ker_log( uint8_t type, sos_pid_t mod_id, uint16_t val )
{
	HAS_CRITICAL_SECTION;
	if( log_started == false ) {
		return;
	}
	
	if( mod_id == KER_LOG_PID || mod_id == KER_UART_PID || 
		mod_id == UART_PID || mod_id == MSG_QUEUE_PID || mod_id == RADIO_PID) {
		return;
	}
	
	ENTER_CRITICAL_SECTION();
	if( log_in_logging == true || log_in_flush == true) {
		LEAVE_CRITICAL_SECTION();
		return;
	}
	log_in_logging = true;
	if( log_buffer == NULL ) {
		log_buffer = ker_malloc( sizeof( sos_log_t ) * SOS_LOG_NUM_BUFFERS, KER_LOG_PID );
		if( log_buffer == NULL ) {
			log_in_logging = false;
			LEAVE_CRITICAL_SECTION();
			return;
		}
		log_index = 0;
	}
	
	log_buffer[log_index].pid = mod_id;
	log_buffer[log_index].type = type;
	log_buffer[log_index].val = val;
	
	log_index++;
	
	if( log_index >= SOS_LOG_NUM_BUFFERS ) {
		ker_log_flush();
	}
	log_in_logging = false;
	LEAVE_CRITICAL_SECTION();
}

void ker_log_flush( void )
{
	sos_log_t *tmp;
	uint8_t idx;
	HAS_CRITICAL_SECTION;
	
	ENTER_CRITICAL_SECTION();
	if( log_index != 0 ) {
		log_in_flush = true;
		tmp = log_buffer;
		idx = log_index;
		log_buffer = NULL;
		log_index = 0;
		post_uart( KER_LOG_PID, KER_LOG_PID, 
			0, sizeof( sos_log_t ) * idx,
			tmp, SOS_MSG_RELEASE, BCAST_ADDRESS);
		log_in_flush = false;
	}
	LEAVE_CRITICAL_SECTION();
}

void ker_log_start()
{
	log_started = true;
}

#endif

