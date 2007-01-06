/**
 * \file printmelf.h
 * \brief Routines to pretty print MELF files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _PRINTMELF_H_
#define _PRINTMELF_H_

#include <minielflib.h>
#include <minielf.h>
#include <libelf.h>

/**
 * \func PrintmelfHdr
 * \brief Print the MELF File header
 * \param MelfDesc MELF Descriptor
 */
int PrintMelfHdr(Melf* MelfDesc);

/**
 * \func PrintMelfSecTable
 * \brief Prints the MELF File Section Table
 * \param MelfDesc MELF Descriptor
 */
int PrintMelfSecTable(Melf* MelfDesc);

/**
 * \func PrintMelfSymTable
 * \brief Prints the MELF File Symbol Table
 * \param MelfDesc MELF Descriptor
 */
int PrintMelfSymTable(Melf* MelfDesc);

/**
 * \func PrintMelfRelaTable
 * \brief Prints the MELF Relocation Table
 * \param MelfDesc MELF Descriptor
 */
int PrintMelfRelaTable(Melf* MelfDesc);

#endif// _PRINTMELF_H_
