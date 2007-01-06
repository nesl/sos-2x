/**
 * \file dispinstr.h
 * \brief Routines to print instructions
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _DISPINSTR_H_
#define _DISPINSTR_H_

int8_t print_optype1(avr_instr_t* instr);
int8_t print_optype2(avr_instr_t* instr);
int8_t print_optype3(avr_instr_t* instr);
int8_t print_optype4(avr_instr_t* instr);
int8_t print_optype5(avr_instr_t* instr);
int8_t print_optype6(avr_instr_t* instr);
int8_t print_optype7(avr_instr_t* instr);
int8_t print_optype8(avr_instr_t* instr);
int8_t print_optype9(avr_instr_t* instr);
int8_t print_optype10(avr_instr_t* instr, avr_instr_t* nextinstr);
int8_t print_optype11(avr_instr_t* instr);
int8_t print_optype12(avr_instr_t* instr);
int8_t print_optype13(avr_instr_t* instr);
int8_t print_optype14(avr_instr_t* instr);
int8_t print_optype15(avr_instr_t* instr);
int8_t print_optype16(avr_instr_t* instr);
int8_t print_optype17(avr_instr_t* instr);
int8_t print_optype18(avr_instr_t* instr);
int8_t print_optype19(avr_instr_t* instr, avr_instr_t* nextinstr);


#endif//_DISPINSTR_H_
