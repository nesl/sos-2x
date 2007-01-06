/**
 * \file avr_melfloader.c
 * \brief Mini-ELF loader specific to AVR architecture
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <avr_minielf.h>
#include <melfloader.h>
#include <codemem.h>

//----------------------------------------------------------
void melf_arch_relocate(melf_desc_t* mdesc, Melf_Rela* rela, Melf_Sym* sym, Melf_Shdr* progshdr)
{
}
