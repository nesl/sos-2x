/**
 * \file minielfendian.c
 * \brief Fix the endian-ness of Mini-ELF data structures
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <minielfendian.h>
//#include <proc_minielf.h>
#include <sos_endian.h>

// ---- ENTOH ------
void entoh_Mhdr(Melf_Mhdr* mhdr)
{
  mhdr->m_shnum = entoh_Melf_Half(mhdr->m_shnum);
  mhdr->m_modhdrndx = entoh_Melf_Word(mhdr->m_modhdrndx);
  return;
}

void entoh_Shdr(Melf_Shdr* shdr)
{
  shdr->sh_id = entoh_Melf_Half(shdr->sh_id);
  shdr->sh_type = entoh_Melf_Half(shdr->sh_type);
  shdr->sh_offset = entoh_Melf_Off(shdr->sh_offset);
  shdr->sh_size = entoh_Melf_Word(shdr->sh_size);
  shdr->sh_link = entoh_Melf_Half(shdr->sh_link);
  shdr->sh_info = entoh_Melf_Half(shdr->sh_info);
  return;
}

void entoh_Sym(Melf_Sym* sym)
{
  sym->st_value = entoh_Melf_Addr(sym->st_value);
  sym->st_shid = entoh_Melf_Half(sym->st_shid);
  return;
}

void entoh_Rela(Melf_Rela* rela)
{
  rela->r_offset = entoh_Melf_Addr(rela->r_offset);
  rela->r_symbol = entoh_Melf_Word(rela->r_symbol);
  rela->r_addend = entoh_Melf_Sword(rela->r_addend);
  return;
}


// ---- EHTON ------
void ehton_Mhdr(Melf_Mhdr* mhdr)
{
  mhdr->m_shnum = ehton_Melf_Half(mhdr->m_shnum);
  mhdr->m_modhdrndx = ehton_Melf_Word(mhdr->m_modhdrndx);
  return;
}

void ehton_Shdr(Melf_Shdr* shdr)
{
  shdr->sh_id = ehton_Melf_Half(shdr->sh_id);
  shdr->sh_type = ehton_Melf_Half(shdr->sh_type);
  shdr->sh_offset = ehton_Melf_Off(shdr->sh_offset);
  shdr->sh_size = ehton_Melf_Word(shdr->sh_size);
  shdr->sh_link = ehton_Melf_Half(shdr->sh_link);
  shdr->sh_info = ehton_Melf_Half(shdr->sh_info);
  return;
}

void ehton_Sym(Melf_Sym* sym)
{
  sym->st_value = ehton_Melf_Addr(sym->st_value);
  sym->st_shid = ehton_Melf_Half(sym->st_shid);
  return;
}

void ehton_Rela(Melf_Rela* rela)
{
  rela->r_offset = ehton_Melf_Addr(rela->r_offset);
  rela->r_symbol = ehton_Melf_Word(rela->r_symbol);
  rela->r_addend = ehton_Melf_Sword(rela->r_addend);
  return;
}


