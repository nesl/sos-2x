/**
 * \file sos_mod_verify.h
 * \brief Verify the integrity of the SOS module
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <sos_mod_patch_code.h>

#ifndef _SOS_MOD_VERIFY_H_
#define _SOS_MOD_VERIFY_H_

/**
 * \addtogroup binverifier
 * Verify safety properties of binary on the node prior to execution
 * @{
 */

#ifdef MINIELF_LOADER

/**
 * Verify the safety properties of a node
 * \param h Handle to codemem component storing the module binary
 */
int8_t ker_verify_module(codemem_t h);

#else
/**
 * Verify the safety properties of a node
 * \param h Handle to codemem component storing the module binary
 * \param init_offset Offset of module handler within the module binary
 * \param code_size Size of binary module
 */
int8_t ker_verify_module(codemem_t h, uint16_t init_offset, uint16_t code_size);
#endif//MINIELF_LOADER

/* @} */

#endif//_SOS_MOD_VERIFY_H_
