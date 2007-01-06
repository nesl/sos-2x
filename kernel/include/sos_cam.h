
#ifndef _SOS_CAM_H_
#define _SOS_CAM_H_

#include <sos_inttypes.h>
/**
 * \addtogroup sosCam
 * @{
 */

/**
 * \file sos_cam.h
 * <STRONG>CAM (Content-addressable memory)</STRONG> is an implementation of 
 * associative array.<BR>  
 * 
 * Based on <A HREF="http://en.wikipedia.org/wiki/Associative_array">Wikepedia</A>,
 * four operations are supported:<BR>
 * -# Add      : Bind a new key to a new value
 * -# Reassign : Bind an old key to a new value
 * -# Remove   : Unbind a key from a vlaue and remove it from the key set
 * -# Lookup   : Find the value (if any) that is bound to a key
 * 
 * \note Implementation does not support duplication.  
 */

/**
 * Type used for the key.  
 *
 * Since there is no support for multiple value of the same key,
 * by convention module should set the upper 8 bits to module ID.  
 */
typedef uint16_t sos_cam_t;

#ifndef _MODULE_
/**
 * Bind a new key to a new value
 * 
 * \param key
 * \param value
 * \return SOS_OK for success, negative for failure (when key already exists)
 */
extern int8_t ker_cam_add(sos_cam_t key, void *value);

/**
 * Bind an old key to a new value
 * 
 * \param key
 * \param new_value
 * \return void*     the old value if exists, NULL if not
 */
extern void* ker_cam_reassign(sos_cam_t key, void *new_value);

/**
 * Unbind a key from a vlaue and remove it from the key set
 * 
 * \param key
 * \return the value that was mapped to the key
 */
extern void* ker_cam_remove(sos_cam_t key);

/**
 * Find the value (if any) that is bound to a key 
 *
 * \param key
 * \return the value mapped to the key
 */
extern void* ker_cam_lookup(sos_cam_t key);
/* @} */
#endif

static inline sos_cam_t ker_cam_key( sos_pid_t pid, uint8_t id ) 
{
  return (((sos_cam_t) pid) << 8) | id;
}



#endif


