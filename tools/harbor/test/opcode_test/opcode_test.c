/**
 * \file opcode_test.c
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \brief Unit test for Opcodes
 */
#include <stdio.h>
#include <inttypes.h>
#include <avrregs.h>
#include <avrinstr.h>
#include <avrinstmatch.h>

//#define OPCODE_TEST_DBG
#ifdef OPCODE_TEST_DBG
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

//-----------------------------------------------------------------------------------
// OPTYPE1 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype1_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t r;
  uint16_t opcode_d;
} optype1_test_t;

#define OPTYPE1_TEST_VEC_SIZE 13
optype1_test_t optype1_test_vec[] = {
  { OP_ADC, R30, R14, OP_ADC},
  { OP_ADD, 32, R12, OP_NOP},
  { OP_ADD, R3, R12, OP_ADD},
  { OP_AND, R1, R3, OP_AND},
  { OP_CP, R4, R19, OP_CP},
  { OP_CPC, R0, R1, OP_CPC},
  { OP_CPSE, R7, R12, OP_CPSE},
  { OP_EOR, 32, R1, OP_NOP},
  { OP_EOR, R2, R1, OP_EOR},
  { OP_MUL, R28, R23, OP_MUL},
  { OP_OR, R16, R31, OP_OR},
  { OP_SBC, R4, R17, OP_SBC},
  { OP_SUB, R16, R26, OP_SUB},
};

int test_optype1()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE1\n");
  for (i = 0; i < OPTYPE1_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype1(optype1_test_vec[i].opcode, 
				 optype1_test_vec[i].d,
				 optype1_test_vec[i].r);
    if (match_optype1(&instr) == 0){
      if (((instr.rawVal & OP_TYPE1_MASK) == optype1_test_vec[i].opcode_d) &&
	  (get_optype1_dreg(&instr) == optype1_test_vec[i].d) &&
	  (get_optype1_rreg(&instr) == optype1_test_vec[i].r)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype1_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}

//-----------------------------------------------------------------------------------
// OPTYPE2 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype2_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t k;
  uint16_t opcode_d;
} optype2_test_t;

#define OPTYPE2_TEST_VEC_SIZE 4
optype2_test_t optype2_test_vec[] = {
  {OP_ADIW, R25, 62, OP_NOP},
  {OP_ADIW, R24, 67, OP_NOP},
  {OP_ADIW, R30, 6, OP_ADIW},
  {OP_SBIW, R28, 49, OP_SBIW},
};

int test_optype2()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE2\n");
  for (i = 0; i < OPTYPE2_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype2(optype2_test_vec[i].opcode, 
				 optype2_test_vec[i].d,
				 optype2_test_vec[i].k);
    if (match_optype2(&instr) == 0){
      if (((instr.rawVal & OP_TYPE2_MASK) == optype2_test_vec[i].opcode_d) &&
	  (get_optype2_dreg(&instr) == optype2_test_vec[i].d) &&
	  (get_optype2_k(&instr) == optype2_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype2_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE3 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype3_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t k;
  uint16_t opcode_d;
} optype3_test_t;

#define OPTYPE3_TEST_VEC_SIZE 7
optype3_test_t optype3_test_vec[] = {
  {OP_ANDI, R25, 232, OP_ANDI},
  {OP_ANDI, R0, 232, OP_NOP},
  {OP_CPI, R24, 67, OP_CPI},
  {OP_LDI, R30, 6, OP_LDI},
  {OP_ORI, R28, 41, OP_ORI},
  {OP_SBCI, R28, 99, OP_SBCI},
  {OP_SUBI, R28, 149, OP_SUBI},
};

int test_optype3()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE3\n");
  for (i = 0; i < OPTYPE3_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype3(optype3_test_vec[i].opcode, 
				 optype3_test_vec[i].d,
				 optype3_test_vec[i].k);
    if (match_optype3(&instr) == 0){
      if (((instr.rawVal & OP_TYPE3_MASK) == optype3_test_vec[i].opcode_d) &&
	  (get_optype3_dreg(&instr) == optype3_test_vec[i].d) &&
	  (get_optype3_k(&instr) == optype3_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype3_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE4 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype4_test {
  uint16_t opcode;
  uint8_t d;
  uint16_t opcode_d;
} optype4_test_t;

#define OPTYPE4_TEST_VEC_SIZE 33
optype4_test_t optype4_test_vec[] = {
  { OP_ASR, R23, OP_ASR},
  { OP_COM, R12, OP_COM},
  { OP_DEC, R9, OP_DEC},
  { OP_INC, R31, OP_INC},
  { OP_ELPM_Z, R28, OP_ELPM_Z},
  { OP_ELPM_Z_INC, R3, OP_ELPM_Z_INC}, 
  { OP_LD_X, R4, OP_LD_X},
  { OP_LD_X_INC, R1, OP_LD_X_INC},
  { OP_LD_X_DEC, R9, OP_LD_X_DEC},
  { OP_LD_Y, R19, OP_LD_Y},
  { OP_LD_Y_INC, R18, OP_LD_Y_INC},
  { OP_LD_Y_DEC, R0, OP_LD_Y_DEC},
  { OP_LD_Z, R7, OP_LD_Z},
  { OP_LD_Z_INC, R22, OP_LD_Z_INC},
  { OP_LD_Z_DEC, R2, OP_LD_Z_DEC},
  { OP_LPM_Z, R23, OP_LPM_Z},
  { OP_LPM_Z_INC, R30, OP_LPM_Z_INC},
  { OP_LSR, R24, OP_LSR},
  { OP_NEG, R16, OP_NEG},
  { OP_POP, R10, OP_POP},
  { OP_PUSH, R4, OP_PUSH},
  { OP_ROR, R1, OP_ROR},
  { OP_ST_X, R22, OP_ST_X},
  { OP_ST_X_INC, R14, OP_ST_X_INC},
  { OP_ST_X_DEC, R22, OP_ST_X_DEC},
  { OP_ST_Y, R26, OP_ST_Y},
  { OP_ST_Y_INC, R21, OP_ST_Y_INC},
  { OP_ST_Y_DEC, R25, OP_ST_Y_DEC},
  { OP_ST_Z, R17, OP_ST_Z},
  { OP_ST_Z_INC, R5, OP_ST_Z_INC},
  { OP_ST_Z_DEC, R3, OP_ST_Z_DEC},
  { OP_SWAP, R16, OP_SWAP},
  { OP_SWAP, 33, OP_NOP},
};

int test_optype4()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE4\n");
  for (i = 0; i < OPTYPE4_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype4(optype4_test_vec[i].opcode, 
				  optype4_test_vec[i].d);
    if (match_optype4(&instr) == 0){
      if (((instr.rawVal & OP_TYPE4_MASK) == optype4_test_vec[i].opcode_d) &&
	  (get_optype4_dreg(&instr) == optype4_test_vec[i].d)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype4_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE5 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype5_test {
  uint16_t opcode;
  uint8_t s;
  uint16_t opcode_d;
} optype5_test_t;

#define OPTYPE5_TEST_VEC_SIZE 3
optype5_test_t optype5_test_vec[] = {
  {OP_BCLR, 7, OP_BCLR},
  {OP_BSET, 0, OP_BSET},
  {OP_BSET, 8, OP_NOP},
};

int test_optype5()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE5\n");
  for (i = 0; i < OPTYPE5_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype5(optype5_test_vec[i].opcode, 
				  optype5_test_vec[i].s);
    if (match_optype5(&instr) == 0){
      if (((instr.rawVal & OP_TYPE5_MASK) == optype5_test_vec[i].opcode_d) &&
	  (get_optype5_s(&instr) == optype5_test_vec[i].s)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype5_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE6 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype6_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t b;
  uint16_t opcode_d;
} optype6_test_t;

#define OPTYPE6_TEST_VEC_SIZE 6
optype6_test_t optype6_test_vec[] = {
  {OP_BLD, R23, 6, OP_BLD},
  {OP_BLD, 33, 4, OP_NOP},
  {OP_BST, R12, 2, OP_BST},
  {OP_SBRC, R19, 9, OP_NOP},
  {OP_SBRC, R31, 6, OP_SBRC},
  {OP_SBRS, R2, 4, OP_SBRS},
};

int test_optype6()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE6\n");
  for (i = 0; i < OPTYPE6_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype6(optype6_test_vec[i].opcode, 
				  optype6_test_vec[i].d,
				  optype6_test_vec[i].b);
    if (match_optype6(&instr) == 0){
      if (((instr.rawVal & OP_TYPE6_MASK) == optype6_test_vec[i].opcode_d) &&
	  (get_optype6_dreg(&instr) == optype6_test_vec[i].d) &&
	  (get_optype6_b(&instr) == optype6_test_vec[i].b)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype6_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE7 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype7_test {
  uint16_t opcode;
  int8_t k;
  uint8_t s;
  uint16_t opcode_d;
} optype7_test_t;

#define OPTYPE7_TEST_VEC_SIZE 6
optype7_test_t optype7_test_vec[] = {
  {OP_BRBC, -62, 6, OP_BRBC},
  {OP_BRBS, 33, 4, OP_BRBS},
  {OP_BRBC, 12, 2, OP_BRBC},
  {OP_BRBS, -19, 6, OP_BRBS},
  {OP_BRBC, -127, 6, OP_NOP},
  {OP_SBRS, 8, 9, OP_NOP},
};

int test_optype7()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE7\n");
  for (i = 0; i < OPTYPE7_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype7(optype7_test_vec[i].opcode, 
				  optype7_test_vec[i].k,
				  optype7_test_vec[i].s);
    if (match_optype7(&instr) == 0){
      if (((instr.rawVal & OP_TYPE7_MASK) == optype7_test_vec[i].opcode_d) &&
	  (get_optype7_k(&instr) == optype7_test_vec[i].k) &&
	  (get_optype7_s(&instr) == optype7_test_vec[i].s)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype7_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE8 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype8_test {
  uint16_t opcode;
  int8_t k;
  uint16_t opcode_d;
} optype8_test_t;

#define OPTYPE8_TEST_VEC_SIZE 34
optype8_test_t optype8_test_vec[] = {
  {OP_BRCC, 1, OP_BRCC}, 
  {OP_BRCS, 2, OP_BRCS}, 
  {OP_BREQ, 3, OP_BREQ}, 
  {OP_BRGE, 5, OP_BRGE}, 
  {OP_BRHC, 45, OP_BRHC}, 
  {OP_BRHS, 34, OP_BRHS}, 
  {OP_BRID, 52, OP_BRID}, 
  {OP_BRIE, 63, OP_BRIE}, 
  {OP_BRLT, 52, OP_BRLT}, 
  {OP_BRMI, 15, OP_BRMI}, 
  {OP_BRNE, 33, OP_BRNE}, 
  {OP_BRPL, 31, OP_BRPL}, 
  {OP_BRTC, 24, OP_BRTC}, 
  {OP_BRTS, 25, OP_BRTS}, 
  {OP_BRVC, 46, OP_BRVC}, 
  {OP_BRVS, 24, OP_BRVS}, 
  {OP_BRCC, -1, OP_BRCC}, 
  {OP_BRCS, -33, OP_BRCS}, 
  {OP_BREQ, -41, OP_BREQ}, 
  {OP_BRGE, -56, OP_BRGE}, 
  {OP_BRHC, -64, OP_BRHC}, 
  {OP_BRHS, -12, OP_BRHS}, 
  {OP_BRID, -9, OP_BRID}, 
  {OP_BRIE, -23, OP_BRIE}, 
  {OP_BRLT, -18, OP_BRLT}, 
  {OP_BRMI, -9, OP_BRMI}, 
  {OP_BRNE, -7, OP_BRNE}, 
  {OP_BRPL, -53, OP_BRPL}, 
  {OP_BRTC, -59, OP_BRTC}, 
  {OP_BRTS, -29, OP_BRTS}, 
  {OP_BRVC, -41, OP_BRVC}, 
  {OP_BRVS, -49, OP_BRVS}, 
  {OP_BRVS, -65, OP_NOP}, 
  {OP_BRVS, 100, OP_NOP}, 
};

int test_optype8()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE8\n");
  for (i = 0; i < OPTYPE8_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype8(optype8_test_vec[i].opcode, 
				  optype8_test_vec[i].k);
    if (match_optype8(&instr) == 0){
      if (((instr.rawVal & OP_TYPE8_MASK) == optype8_test_vec[i].opcode_d) &&
	  (get_optype8_k(&instr) == optype8_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype8_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE10 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype10_test {
  uint16_t opcode;
  uint32_t k;
  uint16_t opcode_d;
} optype10_test_t;

#define OPTYPE10_TEST_VEC_SIZE 4
optype10_test_t optype10_test_vec[] = {
  {OP_CALL, 0xABCDEF10, OP_NOP},
  {OP_JMP, 0x10203040, OP_NOP},
  {OP_CALL, 0x007654, OP_CALL},
  {OP_JMP, 0x003FFFFF, OP_JMP},
};

int test_optype10()
{
  int i;
  int j;
  avr_instr_t instr, nextinstr;
  uint32_t instrval;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE10\n");
  for (i = 0; i < OPTYPE10_TEST_VEC_SIZE; i++){
    instrval = create_optype10(optype10_test_vec[i].opcode, 
				  optype10_test_vec[i].k);
    instr.rawVal = (instrval >> 16);
    nextinstr.rawVal = (uint16_t)(instrval);
    if (match_optype10(&instr) == 0){
      if (((instr.rawVal & OP_TYPE10_MASK) == optype10_test_vec[i].opcode_d) &&
	  (get_optype10_k(&instr, &nextinstr) == optype10_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype10_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE11 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype11_test {
  uint16_t opcode;
  uint8_t a;
  uint8_t b;
  uint16_t opcode_d;
} optype11_test_t;

#define OPTYPE11_TEST_VEC_SIZE 6
optype11_test_t optype11_test_vec[] = {
  {OP_CBI, R23, 6, OP_CBI},
  {OP_CBI, 33, 4, OP_NOP},
  {OP_SBI, R12, 2, OP_SBI},
  {OP_SBIC, R19, 9, OP_NOP},
  {OP_SBIC, R31, 6, OP_SBIC},
  {OP_SBIS, R2, 4, OP_SBIS},
};

int test_optype11()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE11\n");
  for (i = 0; i < OPTYPE11_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype11(optype11_test_vec[i].opcode, 
				  optype11_test_vec[i].a,
				  optype11_test_vec[i].b);
    if (match_optype11(&instr) == 0){
      if (((instr.rawVal & OP_TYPE11_MASK) == optype11_test_vec[i].opcode_d) &&
	  (get_optype11_a(&instr) == optype11_test_vec[i].a) &&
	  (get_optype11_b(&instr) == optype11_test_vec[i].b)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype11_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE12 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype12_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t r;
  uint16_t opcode_d;
} optype12_test_t;

#define OPTYPE12_TEST_VEC_SIZE 8
optype12_test_t optype12_test_vec[] = {
  {OP_FMUL, R23, R16, OP_FMUL},
  {OP_FMULS, R24, R22, OP_NOP},
  {OP_FMULS, R21, R20, OP_FMULS},
  {OP_FMULSU, R19, R18, OP_FMULSU},
  {OP_MULSU, R15, R6, OP_NOP},
  {OP_MULSU, R19, R6, OP_NOP},
  {OP_MULSU, R19, R16, OP_MULSU},
  {OP_FMULSU, R19, R31, OP_NOP},
};

int test_optype12()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE12\n");
  for (i = 0; i < OPTYPE12_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype12(optype12_test_vec[i].opcode, 
				  optype12_test_vec[i].d,
				  optype12_test_vec[i].r);
    if (match_optype12(&instr) == 0){
      if (((instr.rawVal & OP_TYPE12_MASK) == optype12_test_vec[i].opcode_d) &&
	  (get_optype12_d(&instr) == optype12_test_vec[i].d) &&
	  (get_optype12_r(&instr) == optype12_test_vec[i].r)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype12_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE13 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype13_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t a;
  uint16_t opcode_d;
} optype13_test_t;

#define OPTYPE13_TEST_VEC_SIZE 4
optype13_test_t optype13_test_vec[] = {
  {OP_IN, R31, 63, OP_IN},
  {OP_OUT, R0, 0, OP_OUT},
  {OP_IN, 33, 2, OP_NOP},
  {OP_OUT, R19, 69, OP_NOP},
};

int test_optype13()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE13\n");
  for (i = 0; i < OPTYPE13_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype13(optype13_test_vec[i].opcode, 
				  optype13_test_vec[i].d,
				  optype13_test_vec[i].a);
    if (match_optype13(&instr) == 0){
      if (((instr.rawVal & OP_TYPE13_MASK) == optype13_test_vec[i].opcode_d) &&
	  (get_optype13_d(&instr) == optype13_test_vec[i].d) &&
	  (get_optype13_a(&instr) == optype13_test_vec[i].a)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype13_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE14 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype14_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t q;
  uint16_t opcode_d;
} optype14_test_t;

#define OPTYPE14_TEST_VEC_SIZE 6
optype14_test_t optype14_test_vec[] = {
  {OP_LDD_Y, R23, 6, OP_LDD_Y},
  {OP_LDD_Z, 33, 4, OP_NOP},
  {OP_LDD_Z, R12, 2, OP_LDD_Z},
  {OP_STD_Y, R19, 9, OP_STD_Y},
  {OP_STD_Z, R31, 66, OP_NOP},
  {OP_STD_Z, R2, 4, OP_STD_Z},
};

int test_optype14()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE14\n");
  for (i = 0; i < OPTYPE14_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype14(optype14_test_vec[i].opcode, 
				  optype14_test_vec[i].d,
				  optype14_test_vec[i].q);
    if (match_optype14(&instr) == 0){
      if (((instr.rawVal & OP_TYPE14_MASK) == optype14_test_vec[i].opcode_d) &&
	  (get_optype14_d(&instr) == optype14_test_vec[i].d) &&
	  (get_optype14_q(&instr) == optype14_test_vec[i].q)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype14_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE15 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype15_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t r;
  uint16_t opcode_d;
} optype15_test_t;

#define OPTYPE15_TEST_VEC_SIZE 5
optype15_test_t optype15_test_vec[] = {
  {OP_MULS, R23, R6, OP_NOP},
  {OP_MULS, 33, R4, OP_NOP},
  {OP_MULS, R12, R24, OP_NOP},
  {OP_MULS, R19, 33, OP_NOP},
  {OP_MULS, R31, R16, OP_MULS},
};

int test_optype15()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE15\n");
  for (i = 0; i < OPTYPE15_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype15(optype15_test_vec[i].opcode, 
				  optype15_test_vec[i].d,
				  optype15_test_vec[i].r);
    if (match_optype15(&instr) == 0){
      if (((instr.rawVal & OP_TYPE15_MASK) == optype15_test_vec[i].opcode_d) &&
	  (get_optype15_d(&instr) == optype15_test_vec[i].d) &&
	  (get_optype15_r(&instr) == optype15_test_vec[i].r)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype15_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE16 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype16_test {
  uint16_t opcode;
  uint8_t d;
  uint8_t r;
  uint16_t opcode_d;
} optype16_test_t;

#define OPTYPE16_TEST_VEC_SIZE 5
optype16_test_t optype16_test_vec[] = {
  {OP_MOVW, R23, R6, OP_NOP},
  {OP_MOVW, 33, R4, OP_NOP},
  {OP_MOVW, R12, 33, OP_NOP},
  {OP_MOVW, R18, R9, OP_NOP},
  {OP_MOVW, R30, R0, OP_MOVW},
};

int test_optype16()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE16\n");
  for (i = 0; i < OPTYPE16_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype16(optype16_test_vec[i].opcode, 
				  optype16_test_vec[i].d,
				  optype16_test_vec[i].r);
    if (match_optype16(&instr) == 0){
      if (((instr.rawVal & OP_TYPE16_MASK) == optype16_test_vec[i].opcode_d) &&
	  (get_optype16_d(&instr) == optype16_test_vec[i].d) &&
	  (get_optype16_r(&instr) == optype16_test_vec[i].r)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype16_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE17 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype17_test {
  uint16_t opcode;
  int16_t k;
  uint16_t opcode_d;
} optype17_test_t;

#define OPTYPE17_TEST_VEC_SIZE 6
optype17_test_t optype17_test_vec[] = {
  {OP_RCALL, 2047, OP_RCALL},
  {OP_RCALL, -2048, OP_RCALL},
  {OP_RJMP, 189, OP_RJMP},
  {OP_RJMP, -125, OP_RJMP},
  {OP_RCALL, 3000, OP_NOP},
  {OP_RJMP, -2500, OP_NOP},
};

int test_optype17()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE17\n");
  for (i = 0; i < OPTYPE17_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype17(optype17_test_vec[i].opcode, 
				  optype17_test_vec[i].k);
    if (match_optype17(&instr) == 0){
      if (((instr.rawVal & OP_TYPE17_MASK) == optype17_test_vec[i].opcode_d) &&
	  (get_optype17_k(&instr) == optype17_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype17_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE18 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype18_test {
  uint16_t opcode;
  uint8_t d;
  uint16_t opcode_d;
} optype18_test_t;

#define OPTYPE18_TEST_VEC_SIZE 3
optype18_test_t optype18_test_vec[] = {
  {OP_SER, R23, OP_SER},
  {OP_BLD, 33, OP_NOP},
  {OP_SER, R12, OP_NOP},
};

int test_optype18()
{
  int i;
  int j;
  avr_instr_t instr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE18\n");
  for (i = 0; i < OPTYPE18_TEST_VEC_SIZE; i++){
    instr.rawVal = create_optype18(optype18_test_vec[i].opcode, 
				  optype18_test_vec[i].d);
    if (match_optype18(&instr) == 0){
      if (((instr.rawVal & OP_TYPE18_MASK) == optype18_test_vec[i].opcode_d) &&
	  (get_optype18_d(&instr) == optype18_test_vec[i].d)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype18_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}
//-----------------------------------------------------------------------------------
// OPTYPE19 TESTS
//-----------------------------------------------------------------------------------
typedef struct _str_optype19_test {
  uint16_t opcode;
  uint8_t d;
  uint16_t k;
  uint16_t opcode_d;
} optype19_test_t;

#define OPTYPE19_TEST_VEC_SIZE 4
optype19_test_t optype19_test_vec[] = {
  {OP_LDS, R23, 0, OP_LDS},
  {OP_LDS, 33, 4, OP_NOP},
  {OP_STS, R12, 0xffff, OP_STS},
  {OP_STS, 33, 9, OP_NOP},
};

int test_optype19()
{
  int i;
  int j;
  uint32_t instrval;
  avr_instr_t instr, nextinstr;
  j = 0;
  printf("==========================\n");
  printf("Testing OPTYPE19\n");
  for (i = 0; i < OPTYPE19_TEST_VEC_SIZE; i++){
    instrval = create_optype19(optype19_test_vec[i].opcode, 
				  optype19_test_vec[i].d,
				  optype19_test_vec[i].k);
    instr.rawVal = (instrval >> 16);
    nextinstr.rawVal = (uint16_t)(instrval);
    if (match_optype19(&instr) == 0){
      if (((instr.rawVal & OP_TYPE19_MASK) == optype19_test_vec[i].opcode_d) &&
	  (get_optype19_d(&instr) == optype19_test_vec[i].d) &&
	  (get_optype19_k(&nextinstr) == optype19_test_vec[i].k)){
	DEBUG("%d. PASS\n", i);
      }
      else {
	DEBUG("%d. FAIL\n", i);
	j++;
      }
    }
    else if ((instr.rawVal == OP_NOP) && (optype19_test_vec[i].opcode_d == OP_NOP)){
      DEBUG("%d. PASS\n", i);
    }
    else {
      DEBUG("%d. FAIL\n", i);
      j++;
    }
  }
  if (j != 0)
    printf("TEST FAILS (%d times)\n", j);
  else
    printf("TEST PASSED\n");
  printf("==========================\n");
  return 0;
}



int main()
{
  test_optype1();
  test_optype2();
  test_optype3();
  test_optype4();
  test_optype5();
  test_optype6();
  test_optype7();
  test_optype8();
  test_optype10();
  test_optype11();
  test_optype12();
  test_optype13();
  test_optype14();
  test_optype15();
  test_optype16();
  test_optype17();
  test_optype18();
  test_optype19();
  return 0;
}
