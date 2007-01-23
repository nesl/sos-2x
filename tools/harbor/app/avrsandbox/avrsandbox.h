/**
 * \file avrsandbox.h
 * \brief Common data structures and definitions used in sandbox app
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#ifndef _AVR_SANDBOX_H_
#define _AVR_SANDBOX_H_

//----------------------------------------------------------------------------
// DEFINES
//#define SBX_FIX_REGS        //! For binary re-writes assuming fixed registers
#ifdef SBX_FIX_REGS
#define AVR_SCRATCH_REG       R4
#define AVR_FIXED_REG_1       R2
#define AVR_FIXED_REG_2       R3
#else
#define AVR_SCRATCH_REG       R0
#define AVR_FIXED_REG_1       R26   //! Used by the memmap checker routine to store  
#define AVR_FIXED_REG_2       R27   //! the write address
#endif

#define AVR_SREG        0x3F  //! AVR IO address of processor status register
#define BLOCK_SLACK_BYTES 40  //! Slack due to control flow changes 
#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))
#define MAX_SANDBOX_BLK_SIZE 30


//----------------------------------------------------------------------------
// DEBUG
#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

// Control flow update API
void avr_update_cf(bblklist_t* blist, uint32_t startaddr);

// File Write API
void avr_write_binfile(bblklist_t* blist, char* outFileName, file_desc_t *fdesc, uint32_t startaddr);
void avr_write_elffile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr);


#endif//_AVR_SANDBOX_H_
