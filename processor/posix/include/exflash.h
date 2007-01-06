
#ifndef _EXFLASH_H
#define _EXFLASH_H
#include <proc_msg_types.h>
typedef uint16_t exflashpage_t;
typedef uint16_t exflashoffset_t; /* 0 to EXFLASH_PAGE_SIZE - 1 */

int8_t exflash_init();

/**
 * @brief read n bytes from external flash
 * @param pid module id
 * @param page flash page number, MAX = EXFLASH_MAX_PAGES - 1
 * @param offset starting byte in the page
 * @param reqdata data buffer
 * @param n buffer size
 * @return errno
 * @event MSG_EXFLASH_READDONE param->byte = success code (0, 1)
 */
int8_t ker_exflash_read(sos_pid_t pid,
		exflashpage_t page, exflashoffset_t offset,
		void *reqdata, exflashoffset_t n);

/**                                                           
 * @brief write n bytes to external flash                     
 * @param pid module id                                       
 * @param page flash page number, MAX = EXFLASH_MAX_PAGES - 1 
 * @param offset starting byte in the page                    
 * @param reqdata data buffer                                 
 * @param n buffer size                                       
 * @return errno                                              
 * @event MSG_EXFLASH_WRITEDONE param->byte = success code (0, 1)
 */                                                           
int8_t ker_exflash_write(sos_pid_t pid,                       
		exflashpage_t page, exflashoffset_t offset,           
		void *reqdata, exflashoffset_t n);

int8_t ker_exflash_flush(sos_pid_t pid, exflashpage_t page);

int8_t ker_exflash_flushAll(sos_pid_t pid);

#endif

