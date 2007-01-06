/**
 * \file avrinstmatch.h
 * \brief AVR Binary Parser Routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _AVRINSTMATCH_H_
#define _AVRINSTMATCH_H_


#include <inttypes.h>
#include <avrinstr.h>
#include <avrregs.h>

//--------------------------------------------------------------
// OP TYPE1 FUNCTIONS
uint16_t create_optype1(uint16_t opcode, uint8_t d, uint8_t r);
int8_t match_optype1(avr_instr_t* instr);
uint8_t get_optype1_dreg(avr_instr_t* instr);
uint8_t get_optype1_rreg(avr_instr_t* instr);
//--------------------------------------------------------------
// OP TYPE2 FUNCTIONS
uint16_t create_optype2(uint16_t opcode, uint8_t d, uint8_t k);
int8_t match_optype2(avr_instr_t* instr);
uint8_t get_optype2_dreg(avr_instr_t* instr);
uint8_t get_optype2_k(avr_instr_t* instr);
//--------------------------------------------------------------
// OP TYPE3 FUNCTIONS
uint16_t create_optype3(uint16_t opcode, uint8_t d, uint8_t k);
int8_t match_optype3(avr_instr_t* instr);
uint8_t get_optype3_dreg(avr_instr_t* instr);
uint8_t get_optype3_k(avr_instr_t* instr);
//--------------------------------------------------------------
// OP TYPE4 FUNCTIONS
uint16_t create_optype4(uint16_t opcode, uint8_t d);
int8_t match_optype4(avr_instr_t* instr);
uint8_t get_optype4_dreg(avr_instr_t* instr);
//--------------------------------------------------------------
// OP TYPE5 FUNCTIONS
uint16_t create_optype5(uint16_t opcode, uint8_t s);
int8_t match_optype5(avr_instr_t* instr);
uint8_t get_optype5_s(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE6 FUNCTIONS
uint16_t create_optype6(uint16_t opcode, uint8_t d, uint8_t b);
int8_t match_optype6(avr_instr_t* instr);
uint8_t get_optype6_dreg(avr_instr_t* instr);
uint8_t get_optype6_b(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE7 FUNCTIONS
uint16_t create_optype7(uint16_t opcode, int8_t k, uint8_t s);
int8_t match_optype7(avr_instr_t* instr);
int8_t get_optype7_k(avr_instr_t* instr);
uint8_t get_optype7_s(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE8 FUNCTIONS
uint16_t create_optype8(uint16_t opcode, int8_t k);
int8_t match_optype8(avr_instr_t* instr);
int8_t get_optype8_k(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE9 FUNCTIONS
uint16_t create_optype9(uint16_t opcode);
int8_t match_optype9(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE10 FUNCTIONS
uint32_t create_optype10(uint16_t opcode, uint32_t k);
int8_t match_optype10(avr_instr_t* instr);
uint32_t get_optype10_k(avr_instr_t* instr, avr_instr_t* nextinstr);
//-----------------------------------------------------------------------
// OP TYPE11 FUNCTIONS
uint16_t create_optype11(uint16_t opcode, uint8_t a, uint8_t b);
int8_t match_optype11(avr_instr_t* instr);
uint8_t get_optype11_a(avr_instr_t* instr);
uint8_t get_optype11_b(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE12 FUNCTIONS
uint16_t create_optype12(uint16_t opcode, uint8_t d, uint8_t r);
int8_t match_optype12(avr_instr_t* instr);
uint8_t get_optype12_d(avr_instr_t* instr);
uint8_t get_optype12_r(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE13 FUNCTIONS
uint16_t create_optype13(uint16_t opcode, uint8_t d, uint8_t a);
int8_t match_optype13(avr_instr_t* instr);
uint8_t get_optype13_d(avr_instr_t* instr);
uint8_t get_optype13_a(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE14 FUNCTIONS
uint16_t create_optype14(uint16_t opcode, uint8_t d, uint8_t q);
int8_t match_optype14(avr_instr_t* instr);
uint8_t get_optype14_d(avr_instr_t* instr);
uint8_t get_optype14_q(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE15 FUNCTIONS
uint16_t create_optype15(uint16_t opcode, uint8_t d, uint8_t r);
int8_t match_optype15(avr_instr_t* instr);
uint8_t get_optype15_d(avr_instr_t* instr);
uint8_t get_optype15_r(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE16 FUNCTIONS
uint16_t create_optype16(uint16_t opcode, uint8_t d, uint8_t r);
int8_t match_optype16(avr_instr_t* instr);
uint8_t get_optype16_d(avr_instr_t* instr);
uint8_t get_optype16_r(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE17 FUNCTIONS
uint16_t create_optype17(uint16_t opcode, int16_t k);
int8_t match_optype17(avr_instr_t* instr);
int16_t get_optype17_k(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE18 FUNCTIONS
uint16_t create_optype18(uint16_t opcode, uint8_t d);
int8_t match_optype18(avr_instr_t* instr);
uint8_t get_optype18_d(avr_instr_t* instr);
//-----------------------------------------------------------------------
// OP TYPE19 FUNCTIONS
uint32_t create_optype19(uint16_t opcode, uint8_t d, uint16_t k);
int8_t match_optype19(avr_instr_t* instr);
uint8_t get_optype19_d(avr_instr_t* instr);
uint16_t get_optype19_k(avr_instr_t* instr);
#endif//_AVRINSTMATCH_H_
