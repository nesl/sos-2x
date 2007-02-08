/**
 * \file msp430_melfloader.c
 * \brief Mini-ELF loader specific to AVR architecture
 * \author Simon Han
 */
#define STT_FILE 4
#include <melfloader.h>
#include <codemem.h>
#include <fntable.h>

//----------------------------------------------------------
int8_t melf_arch_relocate(melf_desc_t* mdesc, Melf_Rela* rela, Melf_Sym* sym, Melf_Shdr* progshdr)
{
	uint8_t instr[2];

	uint16_t reloc_addr;
	Melf_Addr reloc_offset;

	reloc_offset = (Melf_Addr)(progshdr->sh_offset) + rela->r_offset;
	
	if( MELF_ST_TYPE(sym->st_info) == STT_SOS_DFUNC ) {
		sos_pid_t pid = (sos_pid_t)((sym->st_value >> 8) & 0x00ff);
		uint8_t fid = (uint8_t)(sym->st_value & 0x00ff);
		reloc_addr = (uint16_t)ker_fntable_get_dfunc_addr(pid, fid);
		if( reloc_addr == 0 ) return -1;
	} else {
		reloc_addr = mdesc->base_addr + (uint16_t) progshdr->sh_offset + (uint16_t)sym->st_value + (uint16_t)rela->r_addend;
	}
	
	

	
	//ker_codemem_read(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);
	
	instr[0] = (uint8_t) reloc_addr;
	instr[1] = (uint8_t) (reloc_addr >> 8);

	ker_codemem_write(mdesc->cmhdl, KER_DFT_LOADER_PID, (void*)instr, 2, reloc_offset);

  return 0;
}
//----------------------------------------------------------

