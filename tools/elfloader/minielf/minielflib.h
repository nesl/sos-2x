/**
 * \file minielflib.h
 * \brief Data Structures and API for manipulating Mini-ELF Files
 * \author Ram Kumar {ram@ee.ucla.edu}
 *
 * \defgroup minielflib Mini-ELF Library API
 * @{
 */


#ifndef _MINIELFPRIVATE_H_
#define _MINIELFPRIVATE_H_

#include <minielf.h>
#include <linklist.h>

//---------------------------------------------------
// TYPEDEFS
//---------------------------------------------------

/**
 * \brief Mini-ELF Section Descriptor
 *
 * \note 
 * -# User should not directly access the fields of this data structure
 * -# Make use of the API calls
 */
typedef struct {
  list_t list;      // Doubly Linked List
  Melf_Shdr m_shdr; // Section Header
  void*     m_data; // Data
} Melf_Scn;

/**
 * \brief Mini-ELF Descriptor
 *
 * \note 
 * -# User should not directly access the fields of this data structure
 * -# Make use of the API calls
 */
typedef struct {
  int m_fd;           // File Descriptor of MELF File
  Melf_Mhdr  m_mhdr;  // Melf Header
  Melf_Scn*  m_scn_1; // Link to the first section
  Melf_Scn*  m_scn_n; // Link to the last section
} Melf;


/**
 * \brief Mini-ELF Data Descriptor
 *
 * The data read from Mini-ELF section is accessed through this descriptor.
 *
 * The currently supported ELF data types and their corresponding ELF sections are as follows:
 * -# ELF_T_BYTE: Bytes             - SHT_NOBITS, SHT_PROGBITS, SHT_STRTAB, default
 * -# ELF_T_RELA: Relocation Record - SHT_RELA
 * -# ELF_T_SYM : Symbol Record     - SHT_SYM
 */
typedef struct {
  unsigned char d_type;//!< Describes the type of the data 
  void* d_buf;         //!< Pointer to the actual data buffer
  int   d_size;        //!< Size of the buffer in bytes
  int   d_numData;     //!< Number of elements in the buffer if it contains a list (such as symbol table)
} Melf_Data;


//---------------------------------------------------
// MINI-ELF API
//---------------------------------------------------

/**
 * \defgroup minielfwriteapi Mini-ELF Write API
 * @{
 */

/**
 * \brief Creates a new MELF descriptor for manipulation.
 * \param fd Open file descriptor for the Mini-ELF file.
 * \return MELF (Mini-ELF) descriptor upon success, NULL on failure
 */
Melf* melf_new(int fd);

/**
 * \brief Create a new section
 * \param MelfDesc MELF Descriptor
 * \return MELF section descriptor upon success, NULL on failure
 */
Melf_Scn* melf_new_scn(Melf* MelfDesc);


/**
 * \brief Fills up the data of a section
 * \param scn Section Descriptor to which the data would be written to.
 * \param mdata Data Descriptor of the data being written.
 * \return 0 upon success, -1 upon failure
 */
int melf_add_data_to_scn(Melf_Scn* scn, Melf_Data* mdata);

/**
 * \brief Sort the Mini-ELF sections by the following order
 *  Sorting is done to enable a single-pass patch during load-time.
 *  -# String Table
 *  -# Symbol Table
 *  -# Relocation Sections
 *  -# Other Sections
 *  
 * \param MelfDesc MELF Descriptor
 */
void melf_sortScn(Melf* MelfDesc);

/**
 * \brief Set the offsets of the sections in the MELF file
 * This function should be called just prior to writing the MELF file.
 * \param MelfDesc MELF Descriptor
 */
void melf_setScnOffsets(Melf* MelfDesc);


/**
 * \brief Set the symbol table index of mod_header in the Mini-ELF header
 * \param MelfDesc Mini-ELF Descriptor
 * \param modhdrndx Index of the mod_header symbol in the symbol table section
 */
void melf_setModHdrSymNdx(Melf* MelfDesc, Melf_Word modhdrndx);


/**
 * \brief Writes out the Mini-ELF file corresponding to the MELF descriptor
 * \param MelfDesc MELF Descriptor to be written to file.
 * \return 0 upon success, -1 upon failure
 */
int melf_write(Melf* MelfDesc);
/* @} */


/**
 * \defgroup minielfreadapi Mini-ELF Read API
 * @{
 */

/**
 * \brief Reads the Mini-ELF file and returns a MELF descriptor
 * \param fd File descriptor of the Mini-ELF file
 * \return MELF descriptor upon success, NULL on failure
 */
Melf* melf_begin(int fd);

/**
 * \brief Get the Mini-ELF file header
 * \param MelfDesc MELF Descriptor
 * \return Pointer to the header upon success, NULL on failure
 */
Melf_Mhdr* melf_getmhdr(Melf* MelfDesc);


/**
 * \brief Retreive the section descriptor for a given section ID.
 * \param MelfDesc MELF Descriptor
 * \param sh_id Identity of the section
 * \return Pointer to section descriptor upon success, NULL on failure
 */
Melf_Scn* melf_getscn(Melf* MelfDesc, int sh_id);

/**
 * \brief Retreive the section descriptor following the current one.
 * \param MelfDesc MELF Descriptor
 * \param scn Pointer to the current section descriptor. If scn is NULL, first section descriptor in the file is returned.
 * \return Pointer to section descriptor upon success, NULL on failure
 */
Melf_Scn* melf_nextscn(Melf* MelfDesc, Melf_Scn* scn);

/**
 * \brief Get the identity of the given section descriptor
 * \param scn Pointer to section descriptor
 * \return Section Id upon success, -1 upon failure
 */
int melf_idscn(Melf_Scn* scn);

/**
 * \brief Returns a data descriptor for the section contents 
 * \param scn Section descriptor
 * \return Pointer to data descriptor upon success, NULL upon failure
 */
Melf_Data* melf_getdata(Melf_Scn* scn);


/**
 * \brief Retreives the header of a particular section
 * \param scn Section Descriptor
 * \return Pointer to the section header upon success, NULL upon failure
 */
Melf_Shdr* melf_getshdr(Melf_Scn* scn);

/*@}*/


/*@}*/

#endif//_MINIELFPRIAVTE_H_
