/**
 * \file stringconst.h
 * \brief String constants for pretty printing MELF and ELF files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */


#ifndef _STRINGCONST_H_
#define _STRINGCONST_H_

// Section Type Description
static const char* SecTypeStrTab[]  __attribute__((unused)) = {"NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH", 
				      "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM"};
// Section Flag Description
static const char* SecFlagStrTab[]  __attribute__((unused)) = {"NOF", "W  ", "A  ", "WA ", "X  ", "XW ", "XA ", "XAW"};
// Symbol Binding Description
static const char* SymBindStrTab[]  __attribute__((unused)) = {"LOCAL", "GLOBAL", "WEAK"};
// Symbol Type Description
static const char* SymTypeStrTab[]  __attribute__((unused)) = {"NOTYPE", "OBJECT", "FUNC", "SECTION", "FILE"};




#endif//_STRINGCONST_H_
