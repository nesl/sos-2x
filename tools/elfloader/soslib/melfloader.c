/**
 * \file melfloader.c
 * \brief Mini-ELF loader for SOS
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <string.h>
#include <codemem.h>
#include <minielf.h>
#include <melfloader.h>
#include <minielfendian.h>
#include <flash.h>


//----------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------
static int8_t melf_read_shdr_ndx(melf_desc_t* mdesc, Melf_Shdr* shdr, Melf_Half shndx);
//static int8_t melf_read_shdr_id(melf_desc_t* mdesc, Melf_Shdr* shdr, Melf_Half shid);
static int8_t melf_read_symtab_shdr(melf_desc_t* mdesc, Melf_Shdr* symshdr);
//static int8_t melf_read_relatab_shdr(melf_desc_t* mdesc, Melf_Shdr* relashdr);
static int8_t melf_read_symbol(melf_desc_t* mdesc, Melf_Shdr* symshdr, 
			       Melf_Word symndx, Melf_Sym* sym);
static int8_t melf_read_rela(melf_desc_t* mdesc, Melf_Shdr* relashdr, 
			     Melf_Word relandx, Melf_Rela* rela);
static int8_t melf_relocate(melf_desc_t* mdesc, Melf_Shdr* relashdr, 
			    Melf_Shdr* progshdr, Melf_Shdr* symshdr);




//----------------------------------------------------------
int8_t melf_load_module(codemem_t h)
{
  melf_desc_t mdesc;
  Melf_Shdr symshdr, relashdr, progshdr;
  Melf_Half i;

  // First, get the Mini-ELF Descriptor
  if (melf_begin(&mdesc, h) != SOS_OK)
    return -EFAULT;

  // Read the Section Headers
  for (i = 0; i < mdesc.mhdr.m_shnum; i++){
    Melf_Shdr shdr;
    if (melf_read_shdr_ndx(&mdesc, &shdr, i) != SOS_OK){
      return -EFAULT;
    }
    if (SHT_SYMTAB == shdr.sh_type){
      memcpy(&symshdr, &shdr, sizeof(Melf_Shdr));
    }
    if (SHT_PROGBITS == shdr.sh_type){
      memcpy(&progshdr, &shdr, sizeof(Melf_Shdr));
    }
    if (SHT_RELA == shdr.sh_type){
      memcpy(&relashdr, &shdr, sizeof(Melf_Shdr));
    }    
  }

  // Relocate sections
  melf_relocate(&mdesc, &relashdr, &progshdr, &symshdr);
  return SOS_OK;
}
//----------------------------------------------------------
mod_header_ptr melf_get_header_address(codemem_t h)
{
  melf_desc_t mdesc;
  Melf_Shdr symshdr, progshdr;
  Melf_Sym modhdrsym;
  uint32_t header_byte_addr;

  // First, get the Mini-ELF Descriptor
  if (melf_begin(&mdesc, h) != SOS_OK)
    return 0;
  // Read Prog Bits section header
  if (melf_read_progbits_shdr(&mdesc, &progshdr) != SOS_OK)
    return 0;
  // Read symbol table section header
  if (melf_read_symtab_shdr(&mdesc, &symshdr) != SOS_OK)
    return 0;
  // Read the mod_header symbol
  if (melf_read_symbol(&mdesc, &symshdr, mdesc.mhdr.m_modhdrndx, &modhdrsym) != SOS_OK) 
    return 0;
  header_byte_addr = mdesc.base_addr + progshdr.sh_offset + modhdrsym.st_value;
  return (mod_header_ptr)(FlashGetProgmem(header_byte_addr));
}
//----------------------------------------------------------
int8_t melf_begin(melf_desc_t* mdesc, codemem_t h)
{
  if (ker_codemem_read(h, KER_DFT_LOADER_PID, (void*)&(mdesc->mhdr), sizeof(Melf_Mhdr), 0) != SOS_OK){
    return -EFAULT;
  }
  entoh_Mhdr(&(mdesc->mhdr));
  mdesc->cmhdl = h;
  // Get the base word address of the Mini-ELF module
  mdesc->base_addr = ker_codemem_get_start_address(mdesc->cmhdl);
  return SOS_OK;
}
//----------------------------------------------------------
int8_t melf_read_progbits_shdr(melf_desc_t* mdesc, Melf_Shdr* progshdr)
{
  Melf_Half i;
  for (i = 0; i < mdesc->mhdr.m_shnum; i++){
    if (melf_read_shdr_ndx(mdesc, progshdr, i) != SOS_OK){
      return -EFAULT;
    }
    if (SHT_PROGBITS == progshdr->sh_type){
      return SOS_OK;
    }
  }
  return -EFAULT;
}
//----------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------
static int8_t melf_read_shdr_ndx(melf_desc_t* mdesc, Melf_Shdr* shdr, Melf_Half shndx)
{
  if (shndx >= mdesc->mhdr.m_shnum){
    return -EINVAL;
  }
  if (ker_codemem_read(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)shdr, sizeof(Melf_Shdr), 
		       shndx * sizeof(Melf_Shdr) + sizeof(Melf_Mhdr)) != SOS_OK) {
    return -EFAULT;
  }
  entoh_Shdr(shdr);
  return SOS_OK;
}
//----------------------------------------------------------
/*
static int8_t melf_read_shdr_id(melf_desc_t* mdesc, Melf_Shdr* shdr, Melf_Half shid)
{
  int i;
  for (i = 0; i < mdesc->mhdr.m_shnum; i++){
    if (melf_read_shdr_ndx(mdesc, shdr, i) != SOS_OK){
      return -EFAULT;
    }
    if (shdr->sh_id == shid){
      return SOS_OK;
    }
  }
  return -ESRCH;
}
*/
//----------------------------------------------------------
static int8_t melf_read_symtab_shdr(melf_desc_t* mdesc, Melf_Shdr* symshdr)
{
  Melf_Half i;
  for (i = 0; i < mdesc->mhdr.m_shnum; i++){
    if (melf_read_shdr_ndx(mdesc, symshdr, i) != SOS_OK){
      return -EFAULT;
    }
    if (SHT_SYMTAB == symshdr->sh_type){
      return SOS_OK;
    }
  }
  return -EFAULT;
}
//----------------------------------------------------------
/*
static int8_t melf_read_relatab_shdr(melf_desc_t* mdesc, Melf_Shdr* relashdr)
{
  Melf_Half i;
  for (i = 0; i < mdesc->mhdr.m_shnum; i++){
    if (melf_read_shdr_ndx(mdesc, relashdr, i) != SOS_OK){
      return -EFAULT;
    }
    if (SHT_RELA == relashdr->sh_type){
      return SOS_OK;
    }
  }
  return -EFAULT;
}
*/
//----------------------------------------------------------
static int8_t melf_read_symbol(melf_desc_t* mdesc, Melf_Shdr* symshdr, Melf_Word symndx, Melf_Sym* sym)
{
  if (ker_codemem_read(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)sym, sizeof(Melf_Sym),
		       symshdr->sh_offset + symndx * sizeof(Melf_Sym)) != SOS_OK)
    return -EFAULT;
  entoh_Sym(sym);
  return SOS_OK;
}
//----------------------------------------------------------
static int8_t melf_read_rela(melf_desc_t* mdesc, Melf_Shdr* relashdr, Melf_Word relandx, Melf_Rela* rela)
{
  if (ker_codemem_read(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)rela, sizeof(Melf_Rela),
		       relashdr->sh_offset + relandx * sizeof(Melf_Rela)) != SOS_OK)
    return -EFAULT;
  entoh_Rela(rela);
  return SOS_OK;
}
//----------------------------------------------------------
static int8_t melf_relocate(melf_desc_t* mdesc, Melf_Shdr* relashdr, 
			    Melf_Shdr* progshdr, Melf_Shdr* symshdr)
{
  Melf_Word relandx;
  Melf_Rela rela;
  Melf_Sym sym;

  for (relandx = 0; relandx < (relashdr->sh_size/sizeof(Melf_Rela)); relandx++){
    melf_read_rela(mdesc, relashdr, relandx, &rela);
    melf_read_symbol(mdesc, symshdr, rela.r_symbol, &sym);
    watchdog_reset();
    melf_arch_relocate(mdesc, &rela, &sym, progshdr);
  }
  ker_codemem_flush(mdesc->cmhdl, KER_DFT_LOADER_PID);
  return SOS_OK;
}


