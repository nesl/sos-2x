
#ifndef _SOS_SHM_H_
#define _SOS_SHM_H_

#include <sos_inttypes.h>
/**
 * \addtogroup sosCam
 * @{
 */

/**
 * \file sos_shm.h
 * <STRONG>SHM (Shared memory)</STRONG> is an implementation of 
 * shared memory for SOS.<BR>  
 * 
 * \note Implementation does not support duplication.  
 */

/**
 * Type used for the key.  
 *
 * Since there is no support for multiple value of the same key,
 * by convention module should set the upper 8 bits to module ID.  
 */
typedef uint16_t sos_shm_t;

#ifndef _MODULE_
/**
 * Create a shared memory with the memory 'name' and the shared memory region shm
 * 
 * \param pid  the task ID of the shared memory owner
 * \param name the name of shared memory in numeric value
 * \param shm  the pointer to the shared memory region
 * \return SOS_OK for success, -EEXIST when the name already exists,
 * -ENOMEM for no memory
 */
extern int8_t ker_shm_open(sos_pid_t pid, sos_shm_t name, void *shm);

/**
 * Bind an old key to a new value
 * 
 * \param pid the task ID that updated shared memory
 * \param name the name of shared memory in numeric value
 * \param shm  the pointer to the shared memory region
 * \return SOS_OK for success, -EBADF if the name does not exist
 *
 * ker_shm_update signals the update of the shared memory region.  
 * This could be either a new memory region or the update of existing memory
 */
extern int8_t ker_shm_update(sos_pid_t pid, sos_shm_t name, void *shm);

/**
 * Unbind a key from a vlaue and remove it from the key set
 * 
 * \param pid  the task ID of the shared memory owner
 * \param name the name of shared memory in numeric value
 * \return SOS_OK for success, -EBADF if the name does not exist, -EPERM for incorrect owner
 */
extern int8_t ker_shm_close(sos_pid_t pid, sos_shm_t name);

/**
 * Find the shared memory (if any) that is bound to the name
 *
 * \param pid  the task ID of the requester
 * \param name the name of shared memory in numeric value
 * \return the memory region corresponding to the name, NULL if not exist
 */
extern void* ker_shm_get(sos_pid_t pid, sos_shm_t name);

/**
 * Wait on the event from the shared memory
 *
 * \param pid the task ID of the requester
 * \param name the name of the shared memory in numeric value
 * \return SOS_OK for success, -EBADF for invalid name, -ENOMEM for no space to 
 * store more waiter
 *
 */
extern int8_t ker_shm_wait( sos_pid_t pid, sos_shm_t name );

extern int8_t ker_shm_stopwait( sos_pid_t pid, sos_shm_t name );
/* @} */
#endif

/**
 * Create shared memory name based on task ID and task defined ID 
 */
static inline sos_shm_t sys_shm_name( sos_pid_t pid, uint8_t id ) 
{
  return (((sos_shm_t) pid) << 8) | id;
}


#define SHM_UPDATED    1   // the memory is updated
#define SHM_CLOSED     2   // the memory is closed
static inline sos_shm_t shm_get_name( Message *msg )
{
	MsgParam* params = (MsgParam*)(msg->data);
	return params->word;
}

static inline uint8_t shm_get_event( Message *msg )
{
	MsgParam* params = (MsgParam*)(msg->data);
	
	return params->byte;
}



//
// For kernel usage
//
extern int8_t shm_remove_all( sos_pid_t pid );
extern int8_t shm_init();

#endif


