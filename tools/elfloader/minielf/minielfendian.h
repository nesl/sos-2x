/**
 * \file minielfendian.h
 * \brief Fix the endian-ness of Mini-ELF data structures
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _MINIELFENDIAN_H_
#define _MINIELFENDIAN_H_

#include <minielf.h>

// Fix Endian-ness
// ---- ENTOH ------
void entoh_Mhdr(Melf_Mhdr* mhdr);
void entoh_Shdr(Melf_Shdr* shdr);
void entoh_Sym(Melf_Sym* sym);
void entoh_Rela(Melf_Rela* rela);


// ---- EHTON ------
void ehton_Mhdr(Melf_Mhdr* mhdr);
void ehton_Shdr(Melf_Shdr* shdr);
void ehton_Sym(Melf_Sym* sym);
void ehton_Rela(Melf_Rela* rela);

#endif//_MINIELFENDIAN_H_
