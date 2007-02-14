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
uint32_t find_module_start_addr(file_desc_t* fdesc);


/**
 * \brief Determines if a call or jump instruction contains a relocation record
 * \param fdesc Input File Descriptor
 * \return 0 if the call or jump has a relocation record, -1 otherwise
 */
int8_t check_calljmp_has_reloc_rec(file_desc_t* fdesc, uint32_t callInstrAddr, uint32_t* calltargetaddr);



#endif//_SOS_MOD_HEADER_PATCH_H_
