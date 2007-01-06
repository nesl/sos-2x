/**
 * \file sos_module_header_patch.h
 * \brief Routine to patch the SOS module header
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _SOS_MOD_HEADER_PATCH_H_
#define _SOS_MOD_HEADER_PATCH_H_

/**
 * \brief Patch the module header
 * \param blist List of basic blocks in a program
 * \param mdhr Byte buffer containing the module header
 */
void sos_patch_mod_header(bblklist_t* blist, uint8_t* mhdr);


/**
 * \brief Determine the address of module handler
 * \param fdesc Input file descriptor
 * \return Address of the module handler
 */
uint32_t find_sos_module_handler_addr(file_desc_t* fdesc);


/**
 * \brief Determines if a call instruction calls into system jump table
 * \param fdesc Input File Descriptor
 * \return 0 if the call is into the system jump table, -1 otherwise
 */
int8_t sos_sys_call_check(file_desc_t* fdesc, uint32_t callInstrAddr, uint32_t* calltargetaddr);

#endif//_SOS_MOD_HEADER_PATCH_H_
