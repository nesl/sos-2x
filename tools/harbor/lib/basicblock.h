
/**
 * \file basicblock.h
 * \brief Basic Block Manipulation Library
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _BASICBLOCK_H_
#define _BASICBLOCK_H_

#include <linklist.h>
#include <avrinstr.h>
#include <libelf.h>
#include <fileutils.h>

//--------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------

/**
 * \brief Basic Block Structure
 */
typedef struct _basicblk_str {
  list_t link;                  //!< Linked list of basic blocks
  uint32_t addr;                //!< Start address of the basic block
  uint32_t newaddr;             //!< New start address of the basic block
  uint32_t size;                //!< Size of basic block in bytes
  uint32_t newsize;             //!< New Size Of the Basic Block
  struct _basicblk_str* branch; //!< Link to the branch block
  struct _basicblk_str* fall;   //!< Link to the fall through block
  avr_instr_t* instr;           //!< Array of instructions in the block
  avr_instr_t* newinstr;        //!< Array of sandbox instructions
  uint32_t* addrmap;            //!< Map of old addresses to new addresses (Offsets within block only)
  uint8_t calljmpflag;          //!< Flag set if last instr. is JMP or CALL (Two word CF instr.)
  uint8_t flag;                 //!< User Defined Flags
} basicblk_t;

/**
 * \brief List of basic block
 */
typedef struct _bblk_str {
  basicblk_t* blk_1;            //!< First block in the list of basic blocks
  basicblk_t* blk_n;            //!< Last block in the list of basic blocks
  basicblk_t* blk_st;           //!< Basic block corresponding to the start address of program
  int cnt;                      //!< Number of basic blocks in the program
} bblklist_t;



/**
 * \brief Create a control flow graph representation of a program
 * \param fdesc Input file descriptor
 * \param startaddr Address of the first instruction of program (after all constant tables)
 * \return Pointer to the list of basic block list for the program
 */
bblklist_t* create_cfg(file_desc_t* fdesc, uint32_t startaddr);


/**
 * \brief Display the basic blocks in the program
 * \param blist Pointer to the basic block list of the program
 * \param startaddr Address of the first instruction of program (after all constant tables)
 */
void disp_blocks(bblklist_t* blist, uint32_t startaddr);

/**
 * \brief Display the  list of block boundaries
 * \param blist Pointer to the basic block list of the program
 */
void disp_blkbndlist(bblklist_t* blist);


/**
 * \brief Locate basic block with a specific starting address
 * \param blist Pointer to the basic block list of the program
 * \param addr Starting address of the basic block
 * \return Pointer to the basic block or NULL
 */
basicblk_t* find_block(bblklist_t* blist, uint32_t addr);


/**
 * \brief Return the updated address after sandboxing for a given old address
 */
uint32_t find_updated_address(bblklist_t* blist, uint32_t oldaddr);

#endif//_BASICBLOCK_H_
