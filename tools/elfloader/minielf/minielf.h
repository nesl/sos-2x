/**
 * \file minielf.h
 * \brief Mini-ELF Data Types and File Format
 * \author Ram Kumar {ram@ee.ucla.edu}
 *
 * \defgroup melfloader Mini-ELF Loader Data Types
 *  Mini-Executable and Linkable Format (Mini-ELF) is a relocatable binary file representation
 *  based on the ELF format. 
 *  -# Mini-ELF carries sufficient information to dynamically link and load the binary.
 *  -# Mini-ELF is optimized for size so that it can be tranferred over low power wireless links.
 *  -# Mini-ELF layout is optimized for 8-bit and 16-bit micro-controllers such as AVR, MSP430 etc. 
 * @{
 */


#ifndef _MINIELF_H_
#define _MINIELF_H_

// This will automatically include the processor specific include file
#include <proc_minielf.h>

/**
 * \brief Mini-ELF Header
 *
 * - All Mini-ELF files start with the header.
 * - The header contains global information about the file.
 */
typedef struct{
  Melf_Word     m_modhdrndx; //!< Index of the SOS module header in the symbol table
  Melf_Half     m_shnum;     //!< Number of entries in section table
  unsigned char pad;
} __attribute__((packed)) Melf_Mhdr;


				

/**
 * \brief Mini-ELF Section Header
 *
 * - Section headers follow the Mini-ELF Header
 * - They are organized in the form of a table in the Mini-ELF file.
 */
typedef struct {
  Melf_Half  sh_id;     //!< Unique numeric id of the section
  Melf_Half  sh_type;   //!< Section Type (ELF Section Types are used)
  Melf_Off   sh_offset; //!< Byte offset of the section from the start of file
  Melf_Word  sh_size;   //!< Size of the section in bytes
  Melf_Half  sh_link;   //!< Link to another section (For e.g. Relocation section links to the section for which it contains the relocation information)
  Melf_Half  sh_info;   //!< Section dependant information
} __attribute__((packed)) Melf_Shdr;

/**
 * \brief Mini-ELF Symbol Record
 *
 * - Symbols are organized as an array in a Mini-ELF file.
 * - There is only one symbol table section in a Mini-ELF file
 */
typedef struct {
  Melf_Addr st_value;    //!< Value of the symbol
  unsigned char st_info; //!< Symbol binding and type info. (Same as ELF)
  Melf_Half st_shid;     //!< Section Id containing the symbol
} __attribute__((packed)) Melf_Sym;

/**
 * Macro to retreive binding information from the info field.
 * /note Same as ELF binding information
 */
#define MELF_ST_BIND(i) ((i)>>4)

/**
 * Macro to retreive type information from the info field.
 * /note Same as ELF type information
 */
#define MELF_ST_TYPE(i) ((i)&0x0f)

/**
 * Macro to generate the info field from the binding and type information
 */
#define MELF_ST_INFO(b,t) (((b)<<4)+((t)&0x0f))

/**
 * STT_SOS_DFUNC is the symbol type for direct function linking
 */
#define STT_SOS_DFUNC  (STT_FILE + 1)
/**
 * \brief Mini-ELF Relocation Record
 *
 * - Relocation records are organized as an array in a Mini-ELF file
 * - There is a separate relocation section for every section that needs to be relocated.
 */
typedef struct {
  Melf_Addr r_offset;   //!< Offset from start of section
  Melf_Word r_symbol;   //!< Index into symbol table
  Melf_Sword r_addend;  //!< Addend Value
  unsigned char r_type; //!< Relocation Type
  unsigned char pad;
} __attribute__((packed)) Melf_Rela;

typedef struct sos_func_cb_t {
	Melf_Addr ptr;        //! function pointer                    
	uint8_t proto[4]; //! function prototype                  
	uint8_t pid;      //! function PID                                    
	uint8_t fid;      //! function ID                         
} __attribute__((packed)) 
sos_func_cb_t;

typedef struct sos_mod_header_t {
  uint16_t state_size;      //!< module state size
  uint8_t mod_id;        //!< module ID (used for messaging).  Set NULL_PID for system selected mod_id
  uint8_t num_timers;      //!< Number of timers to be reserved at module load time
  uint8_t num_sub_func;	   //!< number of functions to be subscribed
  uint8_t num_prov_func;   //!< number of functions provided
  uint8_t num_dfunc;       //!< number of direct linked functions
  uint8_t version;         //!< version number, for users bookkeeping
  uint16_t code_id;   //!< module image identifier
  uint8_t processor_type;  //!< processor type of this module
  uint8_t platform_type;   //!< platform type of this module
  uint8_t num_out_port;     //!< Number of output ports exposed by the module in ViRe framework.
  uint8_t padding;          //!< Extra padding to make it word aligned.
  Melf_Addr module_handler;
  sos_func_cb_t funct[];
} __attribute__((packed)) 
sos_mod_header_t;


/*@}*/

#endif//_MINIELF_H_
