
#ifndef _CODEMEM_H
#define _CODEMEM_H

#include <sos_types.h>
#include <codemem_conf.h>
#include <pid.h>
#include <sos_module_types.h>

/**
 * \addtogroup codeMem
 * Codemem is the manager of program memory sections.<BR>
 * The memory section supported by codemem are the following.
 * -# <STRONG>Internal program flash</STRONG><BR>  
 *    Internal program flash allows executable proram to be stored.<BR>  
 *    Internal flash has faster access time than the external flash.
 * -# <STRONG>External data flash (Used to store SOS kernel)</STRONG><BR>
 *    External data flash has slow R/W access.  
 * @{
 */

/**
 * \file codemem.h
 * \brief codemem is the manager of program memory sections.
 * 
 */

/**
 * \brief Codemem Memory Section Type
 */

typedef enum {
	CODEMEM_TYPE_NORMAL = 1,        //!< Non-executable memory section (i.e. external flash memory)
	CODEMEM_TYPE_EXECUTABLE = 2, 	//!< Executable memory section (i.e. internal flash memory)
} codemem_type_t;

enum {
	CODEMEM_INVALID   =   0xffff,   //!< invalid codemem_t
};

typedef uint16_t codemem_t;

#ifndef _MODULE_
/**
 * \brief Allocate a section of memory from codemem
 *
 * \param size           Size of codemem allocation
 * \param type           Type of codemem
 * \return CODEMEM_INVALID when allocation failed.
 * \return codemem_t       when allocation succeeded.
 */
extern codemem_t ker_codemem_alloc(uint16_t size, codemem_type_t type);

/**
 * \brief Write to codemem section
 * \param h        Handle to codemem section
 * \param pid      Requester's pid (used for 
 *                                   sending MSG_EXFLASH_WRITEDONE)
 * \param *buf     Buffer to be written
 * \param nbytes   Number of bytes in buf. Cannot be larger than FLASH_PAGE_SIZE.
 * \param offset   Offset relative to the start of codemem section
 * \return SOS_OK            If write operation succeeded
 * \return SOS_SPLIT         If write requires split phase (typically while writing external flash or
 *                           CODEMEM_TYPE_NORMAL)
 *                           When write is completed, MSG_EXFLASH_WRITEDONE 
 *                           will be sent to the requester.
 */
extern int8_t ker_codemem_write(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset);

/**
 * \brief Read from codemem section
 * \param h        Handle to codemem section
 * \param pid      Requester's pid (used for 
 *                           sending MSG_EXFLASH_READDONE while reading from external flash)
 * \param *buf     Buffer to be read
 * \param nbytes   Number of bytes in buf. Cannot be larger than FLASH_PAGE_SIZE
 * \param offset   Offset relative to the start of codemem section
 * \return SOS_OK            If read operation succeeded
 * \return SOS_SPLIT         If read requires split phase (typically while reading external flash or
 *                           CODEMEM_TYPE_NORMAL)
 *                           When read is completed, MSG_EXFLASH_READDONE 
 *                           will be sent to the requester.
 */
extern int8_t ker_codemem_read(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset);

/**
 * \brief Free codemem section
 * \param h        Handle to codemem section
 * \return SOS_OK  Upon success
 */
extern int8_t ker_codemem_free(codemem_t h);

/**
 * \brief Flush data in the write buffer
 * \param h        Handle to codemem section
 * \param pid      Requester's pid (used for 
 *                           sending MSG_EXFLASH_FLUSHDONE)
 */
extern int8_t ker_codemem_flush(codemem_t h, sos_pid_t pid);

/**
 * \brief Get module header from code ID
 * \param cid code ID
 * \return Module header if found
 * \return 0 if not found
 * 
 * This routine searches the executable section of codemem and tries to 
 * match the code id stored in module header.
 * 
 */
extern mod_header_ptr ker_codemem_get_header_from_code_id( sos_code_id_t cid );


extern int8_t codemem_register_module( mod_header_ptr h );
#endif

/**
 * \brief Mark a codemem section executable. 
 * \param h  Handle to codemem section
 * \return errno
 * \note  h has to be allocated from  CODEMEM_TYPE_EXECUTABLE
 */
extern int8_t ker_codemem_mark_executable(codemem_t h);

extern void codemem_init(void);

#ifndef _MODULE_
extern mod_header_ptr ker_codemem_get_header_address( codemem_t h);
extern uint32_t ker_codemem_get_start_address( codemem_t h);
#define ker_codemem_get_module_code_address(h) ker_codemem_get_header_address(h)
#endif

/* @} */


#endif
