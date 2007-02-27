#ifndef _DVM_CONSTANTS_H_
#define _DVM_CONSTANTS_H_

#include <mod_pid.h>
#include <message_types.h>
#include "codesend.h"

#define ASVM_LIB_MIN_PID	200
#define LIB_ID_BIT			0x80
#define BASIC_LIB_OP_MASK 	0x7F
#define EXT_LIB_OP_MASK 	0x1F
#define EXT_LIB_OP_SHIFT 	5

enum {
  DVM_OPDEPTH      	= 8,
  DVM_BUF_LEN      	= 64, //Ram - This has to be 64 for outlier detection//32,//20
  DVM_CAPSULE_SIZE 	= 28,
  DVM_CAPSULE_NUM	= 8, 
  DVM_CPU_SLICE    	= 50,
  DVM_NUM_SHARED_VARS	= 8,
  DVM_NUM_LOCAL_VARS	= 5,//4
  DVM_NUM_BUFS		= 4,
  DVM_LOCK_COUNT   	= DVM_NUM_SHARED_VARS + DVM_NUM_BUFS, 
  DVM_MAX_SCRIPT_LENGTH = 255,
};

enum {
  DVM_ALL	= 0,
  DVM_STACK	= 1,
  DVM_SCRIPT	= 2,
  DVM_LOCALVARS	= 3,
};

typedef enum {
  DVM_TYPE_NONE      = 0,
  DVM_TYPE_BUFFER    = 1,
  DVM_TYPE_INTEGER   = 3,
  DVM_TYPE_UINT32_LSB= 4,
  DVM_TYPE_UINT32_MSB= 5,
  DVM_TYPE_FLOAT_DEC = 6,
  DVM_TYPE_FLOAT	 = 7,
  DVM_TYPE_MSBPHOTO  = 48,
  DVM_TYPE_MSBTEMP   = 49,
  DVM_TYPE_MSBMIC    = 50,
  DVM_TYPE_MSBMAGX   = 51,
  DVM_TYPE_MSBMAGY   = 52,
  DVM_TYPE_MSBACCELX = 53,
  DVM_TYPE_MSBACCELY = 54,
  DVM_TYPE_THUM      = 55,
  DVM_TYPE_TTEMP     = 56,
  DVM_TYPE_TPAR      = 57,
  DVM_TYPE_TTSR      = 58,
  DVM_TYPE_END       = 59
} DvmDataType;

typedef enum {
  DVM_STATE_HALT,
  DVM_STATE_WAITING,
  DVM_STATE_READY,
  DVM_STATE_RUN,
  DVM_STATE_BLOCKED,
} DvmContextState;

typedef enum {
  DVM_ERROR_TRIGGERED,
  DVM_ERROR_INVALID_RUNNABLE,
  DVM_ERROR_STACK_UNDERFLOW,
  DVM_ERROR_BUFFER_OVERFLOW,
  DVM_ERROR_BUFFER_UNDERFLOW,
  DVM_ERROR_STACK_OVERFLOW,
  DVM_ERROR_INDEX_OUT_OF_BOUNDS,
  DVM_ERROR_INSTRUCTION_RUNOFF,
  DVM_ERROR_LOCK_INVALID,
  DVM_ERROR_LOCK_STEAL,
  DVM_ERROR_UNLOCK_INVALID,
  DVM_ERROR_QUEUE_ENQUEUE,
  DVM_ERROR_QUEUE_DEQUEUE,
  DVM_ERROR_QUEUE_REMOVE,
  DVM_ERROR_QUEUE_INVALID,
  DVM_ERROR_RSTACK_OVERFLOW,
  DVM_ERROR_RSTACK_UNDERFLOW,
  DVM_ERROR_INVALID_ACCESS,
  DVM_ERROR_TYPE_CHECK,
  DVM_ERROR_INVALID_TYPE,
  DVM_ERROR_INVALID_LOCK,
  DVM_ERROR_INVALID_INSTRUCTION,
  DVM_ERROR_INVALID_SENSOR,
  DVM_ERROR_INVALID_HANDLER,
  DVM_ERROR_ARITHMETIC,
  DVM_ERROR_SENSOR_FAIL,
} DvmErrorCode;

/*enum {
  AM_DVMUARTMSG    = (MOD_MSG_START + 40),
  AM_DVMBCASTMSG   = (MOD_MSG_START + 41),
  AM_DVMROUTEMSG         = (MOD_MSG_START + 42),
  AM_DVMVERSIONMSG       = (MOD_MSG_START + 43),
  AM_DVMVERSIONREQUESTMSG= (MOD_MSG_START + 44),
  AM_DVMERRORMSG         = (MOD_MSG_START + 45),
  AM_DVMCAPSULEMSG       = (MOD_MSG_START + 46),
  AM_DVMPACKETMSG        = (MOD_MSG_START + 47),
  AM_DVMCAPSULECHUNKMSG  = (MOD_MSG_START + 48),
  AM_DVMCAPSULESTATUSMSG = (MOD_MSG_START + 49),
};
*/

#define DVM_ERROR_MSG (MOD_MSG_START + 45)

#define TRUE 	1
#define FALSE 	0

//Functions provided by DVM Engine
#define SUBMIT	0
#define ERROR 	1
#define REBOOT 	2 

//Functions provided by Concurrency Manager
#define ANALYZEVARS  		0
#define ANALYZECALLS   		1
#define CLEARANALYSIS   	2
#define INITIALIZECONTEXT   	3
#define YIELDCONTEXT   		4	
#define HALTCONTEXT   		5
#define RESUMECONTEXT   	6
#define ISHELDBY 		7
#define RESET			8

//Functions provided by Handler Store
#define INITIALIZEHANDLER 	0
#define GETCODELENGTH 		1
#define GETOPCODE 		2
#define GETLIBMASK      3
#define GETSTATEBLOCK   4
#define STARTSCRIPTTIMER 5

//Functions provided by MStacks
#define RESETSTACKS 	0
#define PUSHVALUE 	1
#define PUSHBUFFER 	2
#define PUSHOPERAND 	3
#define POPOPERAND 	4
#define SETSTACK 	5

//Functions provided by Resource manager
#define ALLOCATEMEM 	0
#define FREEMEM 	1

//Functions provided by Buffer
#define BUFFER_EXECUTE  0

//Functions provided by Mathlib
#define MATHLIB_EXECUTE 0

//Functions provided by Basiclib
#define BYTELENGTH 	0
#define LOCKNUM 	1
#define REBOOTED 	2
#define SETLOCALVAR 	3
#define EXECUTE 	4

//Functions provided by Queue
#define Q_INIT      0
#define Q_EMPTY     1
#define Q_ENQUEUE   2
#define Q_DEQUEUE   3
#define Q_REMOVE    4

//provided by scriptable modules 
#define EXECUTE_SYNCALL 	0

//May be provided by Typecheck module
#define CHECKTYPES	0
#define CHECKMATCH 	1
#define CHECKVALUE 	2
#define CHECKINTEGER 	3
#define ISINTEGER 	4
#define ISVALUE 	5
#define ISTYPE 		6

//Module IDs in DVM
/*
#define M_CONTEXT_SYNCH 	(ASVM_MOD_MIN_PID + 0)
#define DVM_ENGINE_M 		(ASVM_MOD_MIN_PID + 1)
#define M_STACKS			(ASVM_MOD_MIN_PID + 2)
#define M_HANDLER_STORE 	(ASVM_MOD_MIN_PID + 3)
#define M_BUFFER			(ASVM_MOD_MIN_PID + 4)
#define M_MATHLIB   		(ASVM_MOD_MIN_PID + 5)
#define M_RESOURCE_MANAGER 	(ASVM_MOD_MIN_PID + 6)
#define M_BASIC_LIB 		(ASVM_MOD_MIN_PID + 7)
#define M_QUEUE             (ASVM_MOD_MIN_PID + 8)
*/


//Extension Libraries
#define M_EXT_LIB 		(ASVM_LIB_MIN_PID + 0)
#define M_MATH_LIB 		(ASVM_LIB_MIN_PID + 1)
#define M_RAGO_LIB 		(ASVM_LIB_MIN_PID + 2)


#define MSG_RUN_TASK 		(MOD_MSG_START + 0)
#define MSG_HALT 		(MOD_MSG_START + 1)
#define MSG_RESUME 		(MOD_MSG_START + 2)
#define MSG_VERSION_TIMER_TASK 	(MOD_MSG_START + 3)
#define MSG_CAPSULE_TIMER_TASK 	(MOD_MSG_START + 4)
#define MSG_CHECK_NEED_TASK 	(MOD_MSG_START + 5)
#define MSG_ADD_LIBRARY 	(MOD_MSG_START + 6)
#define MSG_REMOVE_LIBRARY 	(MOD_MSG_START + 7)

#define POST_EXECUTE (MOD_MSG_START + 10)

#define SUBSCRIBE_TIMER 	128
#define ERROR_TIMER 		129
#define VERSION_TIMER 		130
#define CAPSULE_TIMER 		131
#define CLOCK_TIMER 		132

/*
 * MVirus uses the Trickle algorithm for code propagation and maintenance.
 * A full description and evaluation of the algorithm can be found in
 *
 * Philip Levis, Neil Patel, David Culler, and Scott Shenker.
 * "Trickle: A Self-Regulating Algorithm for Code Propagation and Maintenance
 * in Wireless Sensor Networks." In Proceedings of the First USENIX/ACM
 * Symposium on Networked Systems Design and Implementation (NSDI 2004).
 *
 * A copy of the paper can be downloaded from Phil Levis' web site:
 *        http://www.cs.berkeley.edu/~pal/
 *
 * A brief description of the algorithm can be found in the comments
 * at the head of MVirus.c.
 *
 */

typedef enum {
  /* These first two constants define the granularity at which t values
     are calculated (in ms). Version vectors and capsules have separate
     timers, as version timers decay (lengthen) while capsules timers
     are constant, as they are not a continuous process.*/
  MVIRUS_VERSION_TIMER = 100,           // The units of time (ms)
  MVIRUS_CAPSULE_TIMER = 100,           // The units of time (ms)

  /* These constants define how many times a capsule is transmitted,
     the timer interval for Trickle suppression, and the redundancy constant
     k. Due to inherent loss, having a repeat > 1 is preferrable, although
     it should be small. It's better to broadcast the data twice rather
     than require another metadata announcement to trigger another
     transmission. It's not clear whether REDUNDANCY should be > or = to
     REPEAT. In either case, both constants should be small (e.g, 2-4). */
  
  MVIRUS_CAPSULE_REPEAT = 2,            // How many times to repeat a capsule
  MVIRUS_CAPSULE_TAU = 10,              // Capsules have a fixed tau
  MVIRUS_CAPSULE_REDUNDANCY = 2,        // Capsule redundancy (suppression pt.)

  /* These constants define the minimum and maximum tau values for
     version vector exchange, as well as the version vector redundancy
     constant k. Note that the tau values are in terms of multiples
     of the TIMER value above (e.g., a MIN of 10 and a TIMER of 100
     means a MIN of 1000 ms, or one second). */
  MVIRUS_VERSION_TAU_MIN = 10,          // Version scaling tau minimum
  MVIRUS_VERSION_TAU_MAX = 600,         // Version scaling tau maximum
  MVIRUS_VERSION_REDUNDANCY = 1,        // Version redundancy (suppression pt.)
  
  /* These constants are all for sending data larger than a single
     packet; they define the size of a program chunk, bitmasks, etc.*/
  MVIRUS_CAPSULE_HEADER_SIZE = 4,
  MVIRUS_CHUNK_SIZE = DVM_CAPSULE_SIZE + MVIRUS_CAPSULE_HEADER_SIZE,
  MVIRUS_BITMASK_ENTRIES = ((DVM_CAPSULE_SIZE + MVIRUS_CHUNK_SIZE - 1) / MVIRUS_CHUNK_SIZE),
  MVIRUS_BITMASK_SIZE = (MVIRUS_BITMASK_ENTRIES + 7) / 8,
} MVirusConstants;









#endif
