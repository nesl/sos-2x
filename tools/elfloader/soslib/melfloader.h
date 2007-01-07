/**
 * \file melfloader.h
 * \brief Mini-ELF Loader Header File
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#ifndef _MINIELFLOADER_H_
#define _MINIELFLOADER_H_

#include <sos_types.h>
#include <codemem.h>
//#include <proc_minielf.h>
#include <minielf.h>

//----------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------
/**
 * \brief Mini-ELF Descriptor for SOS
 */
typedef struct{
  codemem_t cmhdl;    //!< Code memory handle
  Melf_Mhdr mhdr;     //!< Mini-ELF Header
  uint32_t base_addr; //!< Byte address of the Mini-ELF in memory
} melf_desc_t;




/**
 * \brief Load a Mini-ELF module by relocating the .text section
 * \param h Code memory page handle storing the Mini-ELF module
 * \return 0 upon success, Error code upon failure
 */

int8_t melf_load_module(codemem_t h);

/**
 * \brief Returns the address of the module header
 * \param h Code memory page handle storing the Mini-ELF module
 * \return Word address of the module header upon success, 0 upon failure
 */
mod_header_ptr melf_get_header_address(codemem_t h);

/**
 * \brief Returns the pointer to the module header in a Mini-ELF buffer
 * \param image_buf A byte array of the Mini-ELF image
 * \return Pointer to the module header upon Success, NULL upon failure.
 */
mod_header_t* melf_get_mod_header(unsigned char* image_buf);

/**
 * \brief Architecture specific relocation routine
 * \param mdesc Mini-ELF descriptor
 * \param rela  Relocation Record
 * \param sym   Symbol Record
 * \param progshdr Program Section Header
 */
void melf_arch_relocate(melf_desc_t* mdesc, Melf_Rela* rela, 
			       Melf_Sym* sym, Melf_Shdr* progshdr);

/**
 * \brief Initialize the Mini-ELF Descritpor and read the Mini-ELF header
 * \param mdesc Mini-ELF Descriptor
 * \param h Code memory page hanndle storing the Mini-ELF module
 * \return SOS_OK upon success
 */
int8_t melf_begin(melf_desc_t* mdesc, codemem_t h);


/**
 * \brief Read the program bits section header
 * \param mdesc Initialized Mini-ELF Header
 * \param progshdr Buffer to copy the program section header into
 * \return SOS_OK upon success
 */
int8_t melf_read_progbits_shdr(melf_desc_t* mdesc, Melf_Shdr* progshdr);

// ELF CONSTANTS (from libelf.h)

/*
 * Data types
 */
typedef enum {
  ELF_T_BYTE = 0,	/* must be first, 0 */
  ELF_T_ADDR,
  ELF_T_DYN,
  ELF_T_EHDR,
  ELF_T_HALF,
  ELF_T_OFF,
  ELF_T_PHDR,
  ELF_T_RELA,
  ELF_T_REL,
  ELF_T_SHDR,
  ELF_T_SWORD,
  ELF_T_SYM,
  ELF_T_WORD,
  /*
   * New stuff for 64-bit.
   *
   * Most implementations add ELF_T_SXWORD after ELF_T_SWORD
   * which breaks binary compatibility with earlier versions.
   * If this causes problems for you, contact me.
   */
  ELF_T_SXWORD,
  ELF_T_XWORD,
  /*
   * Symbol versioning.  Sun broke binary compatibility (again!),
   * but I won't.
   */
  ELF_T_VDEF,
  ELF_T_VNEED,
  ELF_T_NUM		/* must be last */
} Elf_Type;


/*
 * sh_type
 */
#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_INIT_ARRAY		14
#define SHT_FINI_ARRAY		15
#define SHT_PREINIT_ARRAY	16
#define SHT_GROUP		17
#define SHT_SYMTAB_SHNDX	18
#define SHT_NUM			19
#define SHT_LOOS		0x60000000
#define SHT_HIOS		0x6fffffff
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

#endif// _MINIELFLOADER_H_
