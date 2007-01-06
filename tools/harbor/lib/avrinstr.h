/**
 * \file avrinstr.h
 * \brief AVR Instruction OpCodes
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _AVRINSTR_H_
#define _AVRINSTR_H_

#include <inttypes.h>

#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))

/**
 * Instruction Types
 */
enum 
  {
    OP_TYPE1 = 1,
    OP_TYPE2,
    OP_TYPE3,
    OP_TYPE4,
    OP_TYPE5,
    OP_TYPE6,
    OP_TYPE7,
    OP_TYPE8,
    OP_TYPE9,
    OP_TYPE10,
    OP_TYPE11,
    OP_TYPE12,
    OP_TYPE13,
    OP_TYPE14,
    OP_TYPE15,
    OP_TYPE16,
    OP_TYPE17,
    OP_TYPE18,
    OP_TYPE19
  };
  

/**
 * Decoder for AVR instruction operands
 */
enum decoder_operand_masks
{
    /** 2 bit register id  ( R24, R26, R28, R30 ) */
    MASK_RD_2 = 0x0030,
    /** 3 bit register id  ( R16 - R23 ) */
    MASK_RD_3 = 0x0070,
    /** 4 bit register id  ( R16 - R31 ) */
    MASK_RD_4 = 0x00f0,
    /** 5 bit register id  ( R00 - R31 ) */
    MASK_RD_5 = 0x01f0,

    /** 3 bit register id  ( R16 - R23 ) */
    MASK_RR_3 = 0x0007,
    /** 4 bit register id  ( R16 - R31 ) */
    MASK_RR_4 = 0x000f,
    /** 5 bit register id  ( R00 - R31 ) */
    MASK_RR_5 = 0x020f,

    /** for 8 bit constant */
    MASK_K_8 = 0x0F0F,
    /** for 6 bit constant */
    MASK_K_6 = 0x00CF,

    /** for 7 bit relative address */
    MASK_K_7 = 0x03F8,
    /** for 12 bit relative address */
    MASK_K_12 = 0x0FFF,
    /** for 22 bit absolute address */
    MASK_K_22 = 0x01F1,

    /** register bit select */
    MASK_REG_BIT = 0x0007,
    /** status register bit select */
    MASK_SREG_BIT = 0x0070,
    /** address displacement (q) */
    MASK_Q_DISPL = 0x2C07,

    /** 5 bit register id  ( R00 - R31 ) */
    MASK_A_5 = 0x00F8,
    /** 6 bit IO port id */
    MASK_A_6 = 0x060F,
};

/****************************************************************************\
 *
 * Helper functions to extract information from the opcodes.
 *
 \***************************************************************************/
static inline uint8_t get_rd_2 (uint16_t opcode)
{
    int reg = ((opcode & MASK_RD_2) >> 4) & 0x3;
    reg = ((reg * 2) + 24);
    return (uint8_t)(reg);
}

static inline uint16_t set_rd_2 (uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = (((uint16_t)reg - 24)/2);
  temp <<= 4;
  temp &= MASK_RD_2;
  temp |= (opcode & ~MASK_RD_2);
  return temp;
}

static inline uint8_t get_rd_3 (uint16_t opcode)
{
    int reg = opcode & MASK_RD_3;
    reg >>= 4;
    reg &= 0x7;
    reg += 16;
    return (uint8_t)(reg);
}

static inline uint16_t set_rd_3(uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = ((uint16_t)reg - 16);
  temp <<= 4;
  temp &= MASK_RD_3;
  temp |= (opcode & ~MASK_RD_3);
  return temp;
}

static inline uint8_t get_rd_4 (uint16_t opcode)
{
    int reg = opcode & MASK_RD_4;
    reg >>= 4;
    reg &= 0xf;
    reg += 16;
    return (uint8_t)(reg);
}

static inline uint16_t set_rd_4(uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = ((uint16_t)reg - 16);
  temp <<= 4;
  temp &= MASK_RD_4;
  temp |= (opcode & ~MASK_RD_4);
  return temp;
}

static inline uint8_t get_rd_5 (uint16_t opcode)
{
    int reg = opcode & MASK_RD_5;
    reg >>= 4;
    reg &= 0x1f;
    return (uint8_t)(reg);
}

static inline uint16_t set_rd_5(uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = (uint16_t)reg;
  temp <<= 4;
  temp &= MASK_RD_5;
  temp |= (opcode & ~MASK_RD_5);
  return temp;
}

static inline uint8_t get_rr_3 (uint16_t opcode)
{
  int reg = (opcode & MASK_RR_3) + 16;
  return (uint8_t)(reg);
}

static inline uint16_t set_rr_3(uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = (uint16_t)reg;
  temp -= 16;
  temp &= MASK_RR_3;
  temp |= (opcode & ~MASK_RR_3);
  return temp;
}

static inline uint8_t get_rr_4 (uint16_t opcode)
{
  int reg = (opcode & MASK_RR_4) + 16;
  return (uint8_t)(reg);
}

static inline uint16_t set_rr_4 (uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = (uint16_t)reg;
  temp -= 16;
  temp &= MASK_RR_4;
  temp |= (opcode & ~MASK_RR_4);
  return temp;
}

static inline uint8_t get_rr_5 (uint16_t opcode)
{
  int reg = opcode & MASK_RR_5;
  reg = (reg & 0xf) + ((reg >> 5) & 0x10);
  return (uint8_t)(reg);
}

static inline uint16_t set_rr_5(uint16_t opcode, uint8_t reg)
{
  uint16_t temp;
  temp = (uint16_t)reg;
  temp = (temp & 0xf) + ((temp & 0x10) << 5);
  temp &= MASK_RR_5;
  temp |= (opcode & ~MASK_RR_5);
  return temp;
}

static inline uint8_t get_k_8 (uint16_t opcode)
{
    int K = opcode & MASK_K_8;
    K = ((K >> 4) & 0xf0) + (K & 0xf);
    return (uint8_t)K;
}

static inline uint16_t set_k_8 (uint16_t opcode, uint8_t K)
{
  uint16_t temp;
  temp = (uint16_t)K;
  temp = (temp & 0xf) + ((temp & 0xf0) << 4);
  temp &= MASK_K_8;
  temp |= (opcode & ~MASK_K_8);
  return temp;
}

static inline uint8_t get_k_6 (uint16_t opcode)
{
    int K = opcode & MASK_K_6;
    K = ((K >> 2) & 0x0030) + (K & 0xf);
    return (uint8_t)K;
}

static inline uint16_t set_k_6 (uint16_t opcode, uint8_t K)
{
  uint16_t temp = (uint16_t)K;
  temp = (temp & 0xf) + ((temp & 0x30) << 2);
  temp &= MASK_K_6;
  temp |= (opcode & ~MASK_K_6);
  return temp;
};


static inline int8_t get_k_7 (uint16_t opcode)
{
  int K;
  K = (((opcode & MASK_K_7) >> 3) & 0x7f);
  if (K & 0x40){
    K |= ~(0x7f);
  }
  return (int8_t)K;
}

static inline uint16_t set_k_7 (uint16_t opcode, int8_t K)
{
  uint16_t temp;
  temp = (uint16_t)(K & 0x7F);
  temp <<= 3;
  temp &= MASK_K_7;
  temp |= (opcode & ~MASK_K_7);
  return temp;
}

static inline int16_t get_k_12 (uint16_t opcode)
{
  int K;
  K = (opcode & MASK_K_12);
  if (K & 0x800){
    K |= ~(0xFFF);
  }
  return (int16_t)K;
}

static inline uint16_t set_k_12 (uint16_t opcode, int16_t K)
{
  uint16_t temp;
  temp = (uint16_t)(K & 0x0FFF);
  temp &= MASK_K_12;
  temp |= (opcode & ~MASK_K_12);
  return temp;
}

static inline uint32_t get_k_22 (uint16_t opcode)
{
    /* Masks only the upper 6 bits of the address, the other 16 bits
     * are in PC + 1. */
    uint32_t k = opcode & MASK_K_22;
    k = ((k >> 3) & 0x003e) + (k & 0x1);
    return k;
}

static inline uint16_t set_k_22 (uint16_t opcode, uint32_t k)
{
  /* Masks only the upper 6 bits of the address, the other 16 bits
   * are in PC + 1. */
  uint16_t temp;
  temp = (uint16_t)((uint32_t)(k >> 16));
  temp = (temp & 0x1) + ((temp & 0x3e) << 3);
  temp &= MASK_K_22;
  temp |= (opcode & ~MASK_K_22);
  return temp;
}

static inline uint8_t get_reg_bit (uint16_t opcode)
{
  uint16_t s;
  s = (opcode & MASK_REG_BIT);
  return (uint8_t)(s);
}

static inline uint16_t set_reg_bit (uint16_t opcode, uint8_t s)
{
  uint16_t temp;
  temp = (uint16_t)s;
  temp &= MASK_REG_BIT;
  temp |= (opcode & ~MASK_REG_BIT);
  return temp;
}

static inline uint8_t get_sreg_bit (uint16_t opcode)
{
  uint16_t sreg;
  sreg = ((opcode & MASK_SREG_BIT) >> 4);
  return (uint8_t)(sreg);
}

static inline uint16_t set_sreg_bit (uint16_t opcode, uint8_t sreg)
{
  uint16_t temp;
  temp = (uint16_t)sreg;
  temp <<= 4;
  temp &= MASK_SREG_BIT;
  temp |= (opcode & ~MASK_REG_BIT);
  return temp;
}

static inline uint8_t get_q (uint16_t opcode)
{
    /* 00q0 qq00 0000 0qqq : Yuck! */
    int q = opcode & MASK_Q_DISPL;
    int qq = (((q >> 1) & 0x1000) + (q & 0x0c00)) >> 7;
    q = (qq & 0x0038) + (q & 0x7);
    return (uint8_t)q;
}

static inline uint16_t set_q (uint16_t opcode, uint8_t q)
{
  uint16_t temp, qq;
  temp = (uint16_t)q;
  qq = temp & 0x0038;
  temp &= 0x7;
  qq = ((qq & 0x0020) << 8) + ((qq & 0x0018) << 7);
  temp |= qq;
  temp &= MASK_Q_DISPL;
  temp |= (opcode & ~MASK_Q_DISPL);
  return temp;
}

static inline uint8_t get_a_5 (uint16_t opcode)
{
  uint16_t a;
  a = ((opcode & MASK_A_5) >> 3);
  return (uint8_t)(a);
}

static inline uint16_t set_a_5 (uint16_t opcode, uint8_t a)
{
  uint16_t temp;
  temp = (uint16_t)a;
  temp <<= 3;
  temp &= MASK_A_5;
  temp |= (opcode & ~MASK_A_5);
  return temp;
}

static inline uint8_t get_a_6 (uint16_t opcode)
{
  uint16_t A = opcode & MASK_A_6;
  A = ((A >> 5) & 0x0030) + (A & 0xf);
  return (uint8_t)(A); 
}

static inline uint16_t set_a_6 (uint16_t opcode, uint8_t a)
{
  uint16_t temp;
  temp = (uint16_t)a;
  temp = ((temp & 0x0030) << 5) + (temp & 0xf);
  temp &= MASK_A_6;
  temp |= (opcode & ~MASK_A_6);
  return temp;
}


typedef struct _str_avrinstr
{
  uint16_t rawVal;
  // OPTYPE 1
#define OP_TYPE1_MASK 0xFC00
#define OP_ADC  0x1C00
#define OP_ADD  0x0C00
#define OP_AND  0x2000
#define OP_CP   0x1400
#define OP_CPC  0x0400
#define OP_CPSE 0x1000
#define OP_EOR  0x2400
#define OP_MOV  0x2C00
#define OP_MUL  0x9C00
#define OP_OR   0x2800
#define OP_SBC  0x0800
#define OP_SUB  0x1800
  /*
    #define OP_CLR 0x2400 // EOR
    #define OP_LSL 0x0C00 // ADD
    #define OP_ROL 0x1C00 // ADC
    #define OP_TST 0x2000 // AND
  */
  // OPTYE 2
#define OP_TYPE2_MASK 0xFF00
#define OP_ADIW 0x9600
#define OP_SBIW 0x9700
  // OPTYPE 3
#define OP_TYPE3_MASK 0xF000
#define OP_ANDI  0x7000
#define OP_CPI   0x3000
#define OP_LDI   0xE000
#define OP_ORI   0x6000
#define OP_SBCI  0x4000
#define OP_SBR   0x6000 //-- Same as OP_ORI
#define OP_SUBI  0x5000
  // OPTYPE 4
#define OP_TYPE4_MASK          0xFE0F
#define OP_ASR                 0x9405
#define OP_COM                 0x9400
#define OP_DEC                 0x940A
#define OP_INC                 0x9403
#define OP_ELPM_Z              0x9006
#define OP_ELPM_Z_INC          0x9007
#define OP_LD_X                0x900C
#define OP_LD_X_INC            0x900D
#define OP_LD_X_DEC            0x900E
#define OP_LD_Y                0x8008
#define OP_LD_Y_INC            0x9009
#define OP_LD_Y_DEC            0x900A
#define OP_LD_Z                0x8000
#define OP_LD_Z_INC            0x9001
#define OP_LD_Z_DEC            0x9002
#define OP_LPM_Z               0x9004
#define OP_LPM_Z_INC           0x9005
#define OP_LSR                 0x9406
#define OP_NEG                 0x9401
#define OP_POP                 0x900F
#define OP_PUSH                0x920F
#define OP_ROR                 0x9407
#define OP_ST_X                0x920C
#define OP_ST_X_INC            0x920D
#define OP_ST_X_DEC            0x920E
#define OP_ST_Y                0x8208
#define OP_ST_Y_INC            0x9209
#define OP_ST_Y_DEC            0x920A
#define OP_ST_Z                0x8200
#define OP_ST_Z_INC            0x9201
#define OP_ST_Z_DEC            0x9202
#define OP_SWAP                0x9402
  // OPTYPE 5
#define OP_TYPE5_MASK 0xFF8F
#define OP_BCLR 0x9488
#define OP_BSET 0x9408
  // OPTYPE 6
#define OP_TYPE6_MASK 0xFE08
#define OP_BLD  0xF800
#define OP_BST  0xFA00
#define OP_SBRC 0xFC00
#define OP_SBRS 0xFE00
  // OPTYPE 7
#define OP_TYPE7_MASK 0xFC00
#define OP_BRBC 0xF800
#define OP_BRBS 0xF000
  // OPTYPE 8
#define OP_TYPE8_MASK 0xFC07
#define OP_BRCC 0xF400
#define OP_BRCS 0xF000
#define OP_BREQ 0xF001
#define OP_BRGE 0xF404
#define OP_BRHC 0xF405
#define OP_BRHS 0xF005
#define OP_BRID 0xF407
#define OP_BRIE 0xF007
#define OP_BRLO 0xF000
#define OP_BRLT 0xF004
#define OP_BRMI 0xF002
#define OP_BRNE 0xF401
#define OP_BRPL 0xF402
#define OP_BRSH 0xF400
#define OP_BRTC 0xF406
#define OP_BRTS 0xF006
#define OP_BRVC 0xF403
#define OP_BRVS 0xF003
  // OPTYPE 9
#define OP_BREAK     0x9598
#define OP_CLC       0x9488
#define OP_CLH       0x94D8
#define OP_CLI       0x94F8
#define OP_CLN       0x94A8
#define OP_CLS       0x94C8
#define OP_CLT       0x94E8
#define OP_CLV       0x94B8
#define OP_CLZ       0x9498
#define OP_EICALL    0x9519
#define OP_EIJMP     0x9419
#define OP_ELPM_Z_R0 0x95D8 //-- R0 is an implicit argument  
#define OP_ICALL     0x9509
#define OP_IJMP      0x9409
#define OP_LPM_Z_R0  0x95C8 //-- R0 is an implicit argument
#define OP_NOP       0x0000
#define OP_RET       0x9508
#define OP_RETI      0x9518
#define OP_SEC       0x9408
#define OP_SEH       0x9458
#define OP_SEI       0x9478
#define OP_SEN       0x9428
#define OP_SES       0x9448
#define OP_SET       0x9468
#define OP_SEV       0x9438
#define OP_SEZ       0x9418
#define OP_SLEEP     0x9588
#define OP_SPM       0x95E8
#define OP_WDR       0x95A8
  // OPTYPE 10
#define OP_TYPE10_MASK 0xFE0E
#define OP_CALL 0x940E
#define OP_JMP  0x940C
  // OPTYPE 11
#define OP_TYPE11_MASK 0xFF00
#define OP_CBI  0x9800
#define OP_SBI  0x9A00
#define OP_SBIC 0x9900
#define OP_SBIS 0x9B00
  // OPTYPE 12
#define OP_TYPE12_MASK 0xFF88
#define OP_FMUL 0x0308
#define OP_FMULS 0x0380
#define OP_FMULSU 0x0388
#define OP_MULSU 0x0300
  // OPTYPE 13
#define OP_TYPE13_MASK 0xF800
#define OP_IN  0xB000
#define OP_OUT 0xB800
  // OPTYPE 14
#define OP_TYPE14_MASK 0xD208
#define OP_LDD_Y 0x8008
#define OP_LDD_Z 0x8000
#define OP_STD_Y 0x8208
#define OP_STD_Z 0x8200
  // OPTYPE 15
#define OP_TYPE15_MASK 0xFF00
#define OP_MULS  0x0200
  // OPTYPE 16
#define OP_TYPE16_MASK 0xFF00
#define OP_MOVW  0x0100
  // OPTYPE 17
#define OP_TYPE17_MASK 0xF000
#define OP_RCALL 0xD000
#define OP_RJMP  0xC000
  // OPTYPE 18
#define OP_TYPE18_MASK 0xFF0F
#define OP_SER  0xEF0F
  // OPTYPE 19
#define OP_TYPE19_MASK 0xFE0F
#define OP_LDS 0x9000
#define OP_STS 0x9200  
} __attribute__((packed)) avr_instr_t;

#if 0
typedef struct _str_avrinstr
{
  union 
  {
    uint16_t rawVal;
    struct
    {
      unsigned op:6;
      unsigned r_field1:1;
      unsigned d:5;
      unsigned r_field2:4;
#define OP_TYPE1_MASK 0xFC00
#define OP_ADC  0x1C00
#define OP_ADD  0x0C00
#define OP_AND  0x2000
#define OP_CP   0x1400
#define OP_CPC  0x0400
#define OP_CPSE 0x1000
#define OP_EOR  0x2400
#define OP_MOV  0x2C00
#define OP_MUL  0x9C00
#define OP_OR   0x2800
#define OP_SBC  0x0800
#define OP_SUB  0x1800
      /*
	#define OP_CLR 0x2400 // EOR
	#define OP_LSL 0x0C00 // ADD
	#define OP_ROL 0x1C00 // ADC
	#define OP_TST 0x2000 // AND
      */
    } __attribute__((packed)) op_type1;
    
    struct
    {
      unsigned op:8;
      unsigned k_field1:2;
      unsigned d:2; // -- Compressed representation pointing to upper four reg pairs
      unsigned k_field2:4;
#define OP_TYPE2_MASK 0xFF00
#define OP_ADIW 0x9600
#define OP_SBIW 0x9700
    } __attribute__((packed)) op_type2;
      
    struct
    {
      unsigned op:4;
      unsigned k_field1:4;
      unsigned d:4; // -- 4 bit reg. representation from r15 to r31.
      unsigned k_field2:4;
#define OP_TYPE3_MASK 0xF000
#define OP_ANDI  0x7000
#define OP_CPI   0x3000
#define OP_LDI   0xE000
#define OP_ORI   0x6000
#define OP_SBCI  0x4000
#define OP_SBR   0x6000 //-- Same as OP_ORI
#define OP_SUBI  0x5000
    } __attribute__((packed)) op_type3;

    struct
    {
      unsigned op_field1:7;
      unsigned d:5;
      unsigned op_field2:4;
#define OP_TYPE4_MASK          0xFE0F
#define OP_ASR                 0x9405
#define OP_COM                 0x9400
#define OP_DEC                 0x940A
#define OP_INC                 0x9403
#define OP_ELPM_Z              0x9006
#define OP_ELPM_Z_INC          0x9007
#define OP_LD_X                0x900C
#define OP_LD_X_INC            0x900D
#define OP_LD_X_DEC            0x900E
#define OP_LD_Y                0x8008
#define OP_LD_Y_INC            0x9009
#define OP_LD_Y_DEC            0x900A
#define OP_LD_Z                0x8000
#define OP_LD_Z_INC            0x9001
#define OP_LD_Z_DEC            0x9002
#define OP_LPM_Z               0x9004
#define OP_LPM_Z_INC           0x9005
#define OP_LSR                 0x9406
#define OP_NEG                 0x9401
#define OP_POP                 0x900F
#define OP_PUSH                0x920F
#define OP_ROR                 0x9407
#define OP_ST_X                0x920C
#define OP_ST_X_INC            0x920D
#define OP_ST_X_DEC            0x920E
#define OP_ST_Y                0x8208
#define OP_ST_Y_INC            0x9209
#define OP_ST_Y_DEC            0x920A
#define OP_ST_Z                0x8200
#define OP_ST_Z_INC            0x9201
#define OP_ST_Z_DEC            0x9202
#define OP_SWAP                0x9402
    } __attribute__((packed)) op_type4;

    struct
    {
      unsigned op_field1:9;
      unsigned s:3;
      unsigned op_field2:4;
#define OP_TYPE5_MASK 0xFF8F
#define OP_BCLR 0x9488
#define OP_BSET 0x9408
    } __attribute__((packed)) op_type5;

    struct
    {
      unsigned op_field1:7;
      unsigned d:5;
      unsigned op_field2:1;
      unsigned b:3;
#define OP_TYPE6_MASK 0xFE08
#define OP_BLD  0xF800
#define OP_BST  0xFA00
#define OP_SBRC 0xFC00
#define OP_SBRS 0xFE00
    } __attribute__((packed)) op_type6;
    
    struct
    {
      unsigned op:6;
      signed k:7;
      unsigned s:3;
#define OP_TYPE7_MASK 0xFC00
#define OP_BRBC 0xF800
#define OP_BRBS 0xF000
    } __attribute__((packed)) op_type7;

    struct
    {
      unsigned op_field1:6;
      signed k:7;
      unsigned op_field2:3;
#define OP_TYPE8_MASK 0xFC07
#define OP_BRCC 0xF400
#define OP_BRCS 0xF000
#define OP_BREQ 0xF001
#define OP_BRGE 0xF404
#define OP_BRHC 0xF405
#define OP_BRHS 0xF005
#define OP_BRID 0xF407
#define OP_BRIE 0xF007
#define OP_BRLO 0xF000
#define OP_BRLT 0xF004
#define OP_BRMI 0xF002
#define OP_BRNE 0xF401
#define OP_BRPL 0xF402
#define OP_BRSH 0xF400
#define OP_BRTC 0xF406
#define OP_BRTS 0xF006
#define OP_BRVC 0xF403
#define OP_BRVS 0xF003
    } __attribute__((packed)) op_type8;
      
    struct
    {
      unsigned op:16;
#define OP_BREAK     0x9598
#define OP_CLC       0x9488
#define OP_CLH       0x94D8
#define OP_CLI       0x94F8
#define OP_CLN       0x94A8
#define OP_CLS       0x94C8
#define OP_CLT       0x94E8
#define OP_CLV       0x94B8
#define OP_CLZ       0x9498
#define OP_EICALL    0x9519
#define OP_EIJMP     0x9419
#define OP_ELPM_Z_R0 0x95D8 //-- R0 is an implicit argument  
#define OP_ICALL     0x9509
#define OP_IJMP      0x9409
#define OP_LPM_Z_R0  0x95C8 //-- R0 is an implicit argument
#define OP_NOP       0x0000
#define OP_RET       0x9508
#define OP_RETI      0x9518
#define OP_SEC       0x9408
#define OP_SEH       0x9458
#define OP_SEI       0x9478
#define OP_SEN       0x9428
#define OP_SES       0x9448
#define OP_SET       0x9468
#define OP_SEV       0x9438
#define OP_SEZ       0x9418
#define OP_SLEEP     0x9588
#define OP_SPM       0x95E8
#define OP_WDR       0x95A8
    } __attribute__((packed)) op_type9;

    struct
    {
      unsigned op_field1:7;
      unsigned k_field1:5;
      unsigned op_field2:3;
      unsigned k_field2:1;
#define OP_TYPE10_MASK 0xFE0E
#define OP_CALL 0x940E
#define OP_JMP  0x940C
    } __attribute__((packed)) op_type10; //-- Two Word Instruction

    struct
    {
      unsigned op:8;
      unsigned a:5;
      unsigned b:3;
#define OP_TYPE11_MASK 0xFF00
#define OP_CBI  0x9800
#define OP_SBI  0x9A00
#define OP_SBIC 0x9900
#define OP_SBIS 0x9B00
    } __attribute__((packed)) op_type11;

    struct
    {
      unsigned op_field1:9;
      unsigned d:3;
      unsigned op_field2:1;
      unsigned r:3;
#define OP_TYPE12_MASK 0xFF88
#define OP_FMUL 0x0308
#define OP_FMULS 0x0380
#define OP_FMULSU 0x0388
#define OP_MULSU 0x0300
    } __attribute__((packed)) op_type12;

    struct
    {
      unsigned op:5;
      unsigned a_field1:2;
      unsigned d:5;
      unsigned a_field2:4;
#define OP_TYPE13_MASK 0xF800
#define OP_IN  0xB000
#define OP_OUT 0xB800
    } __attribute__((packed)) op_type13;


    struct
    {
      unsigned op_field1:2;
      unsigned q_field1:1;
      unsigned op_field2:1;
      unsigned q_field2:2;
      unsigned op_field3:1;
      unsigned d:5;
      unsigned op_field4:1;
      unsigned q_field3:3;
#define OP_TYPE14_MASK 0xD208
#define OP_LDD_Y 0x8008
#define OP_LDD_Z 0x8000
#define OP_STD_Y 0x8208
#define OP_STD_Z 0x8200
    } __attribute__((packed)) op_type14;

    struct
    {
      unsigned op:8;
      unsigned d:4;
      unsigned r:4;
#define OP_TYPE15_MASK 0xFF00
#define OP_MULS  0x0200
    } __attribute__((packed)) op_type15;
    

    struct
    {
      unsigned op:8;
      unsigned d:4;
      unsigned r:4;
#define OP_TYPE16_MASK 0xFF00
#define OP_MOVW  0x0100
    } __attribute__((packed)) op_type16;
    
    struct
    {
      unsigned op:4;
      signed k:12;
#define OP_TYPE17_MASK 0xF000
#define OP_RCALL 0xD000
#define OP_RJMP  0xC000
    } __attribute__((packed)) op_type17;

    struct
    {
      unsigned op_field1:8;
      unsigned d:4;
      unsigned op_field2:4;
#define OP_TYPE18_MASK 0xFF0F
#define OP_SER  0xEF0F
    } __attribute__((packed)) op_type18;

    struct
    {
      unsigned op_field1:7;
      unsigned d:5;
      unsigned op_field2:4;
#define OP_TYPE19_MASK 0xFE0F
#define OP_LDS 0x9000
#define OP_STS 0x9200
    } __attribute__((packed)) op_type19; //--Two Word Instruction


  };
} __attribute__((packed)) avr_instr_t;
#endif

#endif//_AVRINSTR_H_
