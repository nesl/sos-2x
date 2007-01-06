/**
 * \file elfhelper.h
 * \brief Header file for some useful ELF manipulation routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _ELFHELPER_H_
#define _ELFHELPER_H_

#include <libelf.h>

#ifdef __CYGWIN__
#define EM_AVR		83	/* Atmel AVR 8-bit microcontroller */
#define EM_MSP430 	105 	/* Texas Instruments embedded microcontroller msp430 */

/*
 * e_type
 */
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_NUM		5
#define ET_LOOS		0xfe00
#define ET_HIOS		0xfeff
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

#endif//__CYGWIN__



/**
 * \func getELFSymbolTableScn
 * \brief Get the symbol table section
 * \param elf ELF Descriptor
 * \return Pointer to symbol table section, null upon failure
 */
Elf_Scn* getELFSymbolTableScn(Elf* elf);

/**
 * \func getELFSymbolTableNdx
 * \brief Get the ELF symbol table index for a given input symbol
 * \param elf ELF Descriptor
 * \param symName String name of the symbol to be searched
 * \return Index into the symbol table upon success, -1 upon failure
 */
int getELFSymbolTableNdx(Elf* elf, char* symName);

/**
 * \func getELFSectionByName
 * \brief Get the Elf_Scn structure for a given string name of section
 * \param elf ELF Descriptor
 * \param secName String name of the section
 * \return Pointer to section structure upon success, NULL upon failure
 */
Elf_Scn* getELFSectionByName(Elf* elf, char* secName);

/**
 * \func getELFSectionName
 * \brief Gets the string name for a given section index
 * \param elf ELF Descriptor
 * \param ndx Section index
 * \return Pointer to string upon success, NULL upon failure
 */
char* getELFSectionName(Elf* elf, int ndx);

#endif//_ELFHELPER_H_
