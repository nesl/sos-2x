/**
 * \file avr_melfloader.c
 * \brief Mini-ELF loader specific to AVR architecture
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <avr_minielf.h>
#include <melfloader.h>
#include <codemem.h>

static inline void WRITE_LDI(uint8_t* instr, uint8_t byte);

//----------------------------------------------------------
void melf_arch_relocate(melf_desc_t* mdesc, Melf_Rela* rela, Melf_Sym* sym, Melf_Shdr* progshdr)
{
  uint8_t instr[4];
  uint32_t reloc_addr;
  Melf_Addr reloc_offset;
  
  reloc_offset = (Melf_Addr)(progshdr->sh_offset) + rela->r_offset;
  
  reloc_addr = mdesc->base_addr + (uint32_t) progshdr->sh_offset + (uint32_t)sym->st_value + (uint32_t)rela->r_addend;

  ker_codemem_read(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 4, reloc_offset);
  
  switch (rela->r_type) {
  case R_AVR_NONE:
  case R_AVR_32:
  case R_AVR_7_PCREL:
  case R_AVR_13_PCREL:
    return;
    
  case R_AVR_16:
    instr[0] = (uint8_t)reloc_addr;
    instr[1] = (uint8_t)(reloc_addr >> 8);
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;
    
  case R_AVR_16_PM:
    reloc_addr = (reloc_addr >> 1);
    instr[0] = (uint8_t)reloc_addr;
    instr[1] = (uint8_t)(reloc_addr >> 8);
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_LO8_LDI:
    WRITE_LDI(instr, (uint8_t)reloc_addr);
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HI8_LDI:
    WRITE_LDI(instr, (uint8_t)(reloc_addr >> 8));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HH8_LDI:
    WRITE_LDI(instr, (uint8_t)(reloc_addr >> 16));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;
    
  case R_AVR_LO8_LDI_NEG:
    WRITE_LDI(instr, (uint8_t)(-reloc_addr));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HI8_LDI_NEG:
    WRITE_LDI(instr, (uint8_t)((-reloc_addr) >> 8));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;
    
  case R_AVR_HH8_LDI_NEG:
    WRITE_LDI(instr, (uint8_t)((-reloc_addr) >> 16));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_LO8_LDI_PM:
    WRITE_LDI(instr, (uint8_t)(reloc_addr >> 1));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HI8_LDI_PM:
    WRITE_LDI(instr, (uint8_t)(reloc_addr >> 9));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HH8_LDI_PM:
    WRITE_LDI(instr, (uint8_t)(reloc_addr >> 17));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_LO8_LDI_PM_NEG:
    WRITE_LDI(instr, (uint8_t)((-reloc_addr) >> 1));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HI8_LDI_PM_NEG:
    WRITE_LDI(instr, (uint8_t)((-reloc_addr) >> 9));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_HH8_LDI_PM_NEG:
    WRITE_LDI(instr, (uint8_t)((-reloc_addr) >> 17));
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);    
    break;

  case R_AVR_CALL:
	//
    reloc_addr = (reloc_addr >> 1);
    instr[2] = (uint8_t)(reloc_addr);
    instr[3] = (uint8_t)(reloc_addr >> 8);
    ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 4, reloc_offset);
    break;

  default:
    break;
  }

  return;
}
//----------------------------------------------------------
static inline void WRITE_LDI(uint8_t* instr, uint8_t byte)
{							
  instr[0] = (instr[0] & 0xf0) | (byte & 0x0f);	
  instr[1] = (instr[0] & 0xf0) | (byte >> 4);
  return;
}
