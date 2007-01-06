/**
 * \file elfdisplay.h
 * \brief Functions to pretty print ELF files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _ELFDISPLAY_H_
#define _ELFDISPLAY_H_


/**
 * \func ElfPrintReloc
 * \brief Print the ELF Relocation Table
 * \param elf ELF Descriptor
 */
int ElfPrintReloc(Elf* elf);

/**
 * \func ElfPrintHeader
 * \brief Print the ELF Header
 * \param elf ELF Descriptor
 */
int ElfPrintHeader(Elf32_Ehdr *ehdr, 
		   int archNum[], int archStrTablLen, int archNumNdx, char* archStrTab[]);


/**
 * \func ElfPrintSections
 * \brief Print all ELF section headers
 * \param elf ELF Descriptor
 */
int ElfPrintSections(Elf *elf);

/**
 * \func ElfPrintSymTab
 * \brief Print ELF symbol table
 * \param elf ELF Descriptor
 */
int ElfPrintSymTab(Elf *elf);


/**
 * \func ElfPrintSymbol
 * \brief Print ELF Symbol
 * \param esym Pointer to symbol
 */
int ElfPrintSymbol(Elf32_Sym *esym);

#endif//_ELFDISPLAY_H_
