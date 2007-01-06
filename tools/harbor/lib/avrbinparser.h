/**
 * \file avrbinparser.h
 * \brief Header file for the AVR binparser
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _AVRBINPARSER_H_
#define _AVRBINPARSER_H_

#include <avrinstr.h>

int8_t decode_avr_instr_word(avr_instr_t* instr);

#endif//_AVRBINPARSER_H_
