/**
 * \file malloc_extmem.h
 * \brief Header for the external memory manager
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <sos_inttypes.h>
#include <pid.h>
#ifndef _MALLOC_EXTMEM_H_
#define _MALLOC_EXTMEM_H_

//-------------------------------------------------
// CONSTANTS
//-------------------------------------------------
// Ram - Do not change the memory layout
// The permissions table has been setup for this layout only
enum
  {
    EXT_MEM_START           = 0x1100,
    EXT_MEM_HEAP_START      = 0x1100, // Same as EXT_MEM_START
    EXT_MEM_HEAP_END        = 0xFBFF,
    //EXT_MEMMAP_TABLE_START  = 0xFC00, 
    EXT_MEM_HANDLE_START    = 0xFC00, 
    EXT_MEM_END             = 0xFFFF,

    EXT_MEM_BLK_SIZE        = 256,
    EXT_MEM_SHIFT_SIZE      = 8,
    EXT_MEM_NUM_BLKS        = 235,
    EXT_MEM_MAX_SEG         = 100,    // These are the max no. of allocated segments
  };

//-------------------------------------------------
// INTERNAL KERNEL FUNCTIONS
//-------------------------------------------------
// Get handle to an external memory block
int16_t ext_mem_get_handle(uint16_t size, sos_pid_t id, bool bCallFromModule);
// Free Handle
int8_t ext_mem_free_handle(int16_t handle, bool bCallFromModule);


#ifndef _MODULE_
//#define _MODULE_

  
  
// Initialize external memory heap
int8_t ext_mem_init();
//-------------------------------------------------
// KERNEL API
//-------------------------------------------------
/*
 * @brief allocate a memory space in the external RAM
 * @param size in bytes and module ID of the caller
 * @return handle which hashes to a pointer that points to the location of the image
 */
static inline int16_t ker_get_handle(uint16_t size, sos_pid_t id)
{
  return ext_mem_get_handle(size, id, false);
}
/*
 * @brief free the memory allocated for an image
 * @param handle for the image
 * @return SOS_OK if succesful, 
 * @return -EINVAL if fails 
 */
static inline int8_t ker_free_handle(int16_t handle)
{
  return ext_mem_free_handle(handle, false);
}

/*
 * @brief retrieve the pointer pointing to the image stored in the external RAM
 * @param handle for the image
 * @return pointer to the image
 */
void* ker_get_handle_ptr(int16_t handle);


#endif//_MODULE_
#endif//_MALLOC_EXTMEM_H_
