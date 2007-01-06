/**
 * @brief    PIDs of kernel, device and application modules
 * @author   Simon Han (simonhan@ee.ucla.edu)
 * @version  0.1
 *
 */
#ifndef _PID_H
#define _PID_H

#include <sos_inttypes.h>

typedef uint8_t sos_pid_t;

/**
 * @brief native kernel device
 * For devices that are native in the kernel, 
 * there is no handler associated with it.  
 * We provide a number for application to identify the source of message
 * we identify them in the comment as NO_HANDLE
 * 
 */
enum {
	/* 00 */	RSVD_0_PID=0,       //!<
	/* 01 */	RSVD_1_PID,         //!<
	/* 02 */	KER_SCHED_PID,      //!< pid for scheduler, NO_HANDLE
	/* 03 */	KER_MEM_PID,        //!< pid for malloc daemon
	/* 04 */	TIMER_PID,          //!< pid for timer daemon, NO_HANDLE
	/* 05 */	ADC_PID,            //!< pid for adc daemon, NO_HANDLE
	/* 06 */	KER_SENSOR_PID,     //!< pid for sensor daemon
	/* 07 */	USER_PID,           //!< pid for user input, NO_HANDLE
	/* 08 */	KER_LOG_PID,        //!< pid for kernel logging 
	/* 09 */	RADIO_PID,          //!< pid for radio
	/* 10 */	MONITOR_PID,        //!< pid for monitoring daemon
	/* 11 */	MSG_QUEUE_PID,      //!< pid for message queue daemon
	/* 12 */	FNTABLE_PID,        //!< pid for module fn pointer pointer daemon
	/* 13 */	SOSBASE_PID,        //!< pid for SOSBase monitor
	/* 14 */	KER_TS_PID,         //!< pid for time stamp service
	/* 15 */	KER_CODEMEM_PID,    //!< pid for codemem
	/* 16 */	KER_FETCHER_PID,    //!< pid for reliable code fetcher
	/* 17 */	KER_DFT_LOADER_PID, //!< pid for default loader (it loads everything)
	/* 18 */    KER_SPAWNER_PID,    //!< pid for server that spawns module
	/* 19 */    KER_CAM_PID,        //!< pid for cam 
	/* 255 */	NULL_PID           = 255, //!< pid to indicate module does not exist
};
// PLEASE add the string to kernel/pid.c

#define SYS_MAX_PID 20

enum {
	KER_MOD_MAX_PID    = 63,      //! highest pid kernel module can use
	DEV_MOD_MIN_PID    = 64,      //! pids for processor or platform devices
	APP_MOD_MIN_PID    = 128,     //! pids for applications
	APP_MOD_MAX_PID	   = 223,     //! max pid
	SOS_MAX_PID        = 254,     
};

/**
 * The inlucdes below depend the defines above.
 */

#ifndef NO_MOD_PID
#include <mod_pid.h>
#endif

#ifndef NO_PID_PROC
#include <pid_proc.h>
#endif

#ifndef NO_PID_PLAT
#include <pid_plat.h>
#endif

#ifdef PC_PLATFORM
extern char ker_pid_name[][256];
#endif 

#endif

