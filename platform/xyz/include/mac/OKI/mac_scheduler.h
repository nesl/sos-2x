/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                                 Task Scheduler                                 *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * CONFIDENTIAL                                                                                        *
 * The use of this file is restricted by the signed MAC software license agreement.                    *
 *                                                                                                     *
 * Copyright Chipcon AS, 2004                                                                          *
 *******************************************************************************************************
 * This module contains the task scheduler used by the MAC layer. A task can begin at every backoff    *
 * slot boundary, assuming that no tasks of same or higher priorities running:                         *
 *                                                                                                     *
 *                        Task queues:     Task pool:                                                  *
 *                     |  H+<T             |   TT   |                                                  *
 * Execution priority: |  H <TTT    <----- |TT TTT T|                                                  *
 *                     |  M <TT            |T TT TTT|                                                  *
 *                     V  L <TTTTT         |--------|                                                  *
 *                           ^                                                                         *
 *                           |                                                                         *
 *                           One-way linked list, FIFO                                                 *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_SCHEDULER_H
#define MAC_SCHEDULER_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The total number of tasks in the pool
#define MAC_TASK_COUNT                  20

// The number of priorities/queues
#define MAC_TASK_PRIORITY_COUNT         4

// Priority names (MAC_TASK_PRIORITY_COUNT = 4) 
#define MAC_TASK_PRI_HIGHEST            3
#define MAC_TASK_PRI_HIGH               2
#define MAC_TASK_PRI_MEDIUM             1
#define MAC_TASK_PRI_LOW                0

// The initial task state (do not change)
#define MAC_TASK_STATE_INITIAL          0

// Queue terminator
#define NO_TASK                         0xFF
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Task removal flags

// Don't release the task back to the pool when it is removed from the queue
#define MSCH_KEEP_TASK_RESERVED_BM      0x01

// Don't start other tasks of same or lower priority until the removed task has returned
#define MSCH_KEEP_TASK_IN_PROGRESS_BM   0x02
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Task queues
typedef struct {
    UINT8 firstTask;
    UINT8 lastTask;
    BOOL taskInProgress;
} MAC_TASK_QUEUE;
extern MAC_TASK_QUEUE pMacTaskQueues[MAC_TASK_PRIORITY_COUNT];

// Tasks/pool
typedef struct {
    void *pTaskFunc;
    UWORD taskData;
    UBYTE state;
    volatile BOOL occupied;
    volatile UINT8 nextTask;
    UINT8 priority;
    UINT8 taskNumber;
} MAC_TASK_INFO;
extern MAC_TASK_INFO pMacTasks[MAC_TASK_COUNT];

// Task function pointer type (void TaskFunc(MAC_TASK_INFO *pTask))
typedef void (*TFPTR)(MAC_TASK_INFO *pTask);
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Initializes the scheduler (must be called before the MAC timer is started)
void mschInit(void);

// Reserve/release
UINT8 mschReserveTask(void);
void mschReleaseTask(UINT8 taskNumber);

// Task queue management
BOOL mschAddTask(UINT8 taskNumber, UINT8 priority, TFPTR pTaskFunc, UWORD taskData);
void mschRemoveTask(UINT8 priority, BOOL release);
void mschRescheduleTask(MAC_TASK_INFO *pTask, UINT8 state);

// Task execution (called by the timing module)
void mschDoTask(void);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_scheduler.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:50  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:29  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:01  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.4  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
