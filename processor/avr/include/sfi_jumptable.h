/**
 * \file sfi_jumptable.h
 * \brief Jump table implementation for SOS kernel in SFI mode
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <inttypes.h>
#include <codemem_conf.h> // For CODEMEM_END_PAGE
#include <sos_module_types.h>
#include <sos_linker_conf.h>
#ifdef MINIELF_LOADER
#include <codemem.h>
#endif
#ifndef _SFI_JUMPTABLE_H_
#define _SFI_JUMPTABLE_H_

/**
 * \addtogroup sfijumptable
 * <STRONG>SFI Jumptable Organization</STRONG> <BR>
 * Pages: (CODEMEM_END_PAGE + 1) - (CODEMEM_END_PAGE + 10) <BR>
 * Kernel: 2 pages <BR>
 * Modules: 7 pages <BR>
 * 1 extra page left at the end <BR>
 * SFI Jumptable contains instructions: jmp <addr> <BR>
 * Similar in design to the interrupt table <BR>
 * The SFI table for the kernel is created at compile time by a script <BR>
 * The SFI table for each module is created at load-time <BR>
 * @{
 */



/**
 * Configuration for SFI Jumptable
 */
// Ram - Modify the AVR Makerules to move .sfijmptbl to the page SFI_JUMP_TABLE_START
// Ram - Modify create_sfijumptable.py if the order of tables is shuffled
enum sfi_jumptable_config_t
  {
    SFI_JUMP_TABLE_START = (CODEMEM_END_PAGE + 1),   //! First page of SFI Table
    SFI_JUMP_TABLE_END   = (CODEMEM_END_PAGE + 10),  //! Last page of SFI Table
    SFI_KER_TABLE_0 = SFI_JUMP_TABLE_START,      
    SFI_KER_TABLE_1,
    SFI_DOM0_TABLE, 
    SFI_DOM1_TABLE,
    SFI_DOM2_TABLE,
    SFI_DOM3_TABLE,
    SFI_DOM4_TABLE,
    SFI_DOM5_TABLE,
    SFI_DOM6_TABLE,
    SFI_PAD_TABLE, //!< Page for padding: Makes SFI table end on an odd page
  };

/**
 * Number of protection domains supported
 */
#define SFI_MOD_DOMAINS 7 // There are 7 module protection domains and a kernel protection domain


/**
 * Domain Identities
 */
enum sfi_domain_id_t
  {
    SFI_DOM0 = 0, //! Domain 0
    SFI_DOM1,     //! Domain 1
    SFI_DOM2,     //! Domain 2
    SFI_DOM3,     //! Domain 3
    SFI_DOM4,     //! Domain 4
    SFI_DOM5,     //! Domain 5
    SFI_DOM6,     //! Domain 6
  };


#define WORDS_PER_PAGE 128L
#define SIZE_OF_JMP_INSTR 2 // 2 word instruction
#define SFI_KER_TABLE_OFFSET (SFI_KER_TABLE_0 * WORDS_PER_PAGE)
#define SFI_KER_TABLE_ENTRY(k) (void*)(SFI_KER_TABLE_OFFSET + (SIZE_OF_JMP_INSTR * (k)))



/**
 * Initialize the module jump table
 */

int8_t sfi_modtable_init();


/**
 * Register a module with the jump table. This will setup a jumptable for the module.
 * \param mhdr Header for the module being registered
 */
#ifdef MINIELF_LOADER
void sfi_modtable_register(codemem_t h);
#else
void sfi_modtable_register(mod_header_t* mhdr);
#endif



/**
 * De-register a module with the SFI jump table.
 * \param pid Id of module being de-registered
 */
void sfi_modtable_deregister(sos_pid_t pid);

/**
 * Adds a given function address to the jump table and returns the address of the jump table location
 * \param addr Real address of a function that is to be added to the jumptable
 * \return The location in the jumptable where the function address is being added
 */
func_addr_t sfi_modtable_add(func_addr_t addr);

/**
 * Writes the jumptable into the flash
 * \param h The module header whose jump table is being written
 */
void sfi_modtable_flash(mod_header_ptr h);

/**
 * Get the real address of a dynamic function from the jump table
 * \param addr  The address pointing to the entry in the jump table
 * \return The real address of the function from the jump table
 */
func_addr_t sfi_modtable_get_real_addr(func_addr_t addr);

/**
 * Get the domain ID for a given module ID
 * \param pid Module ID
 * \return -1 if Module ID is not assigned domain
 */
int8_t sfi_get_domain_id(sos_pid_t pid);


/**
 * \brief Exception routine for SFI Jumptable violations
 */
void sfi_jmptbl_exception();


/**
 * \brief Exception routine displays error code of exception and halts system
 * \param errcode Error code indicates type of SFI exception
 */
void sfi_exception(uint8_t errcode);


/**
 * \brief Initialize the SFI Kernel Table
 */
void sfi_kertable_init() __attribute__((section(".sfi_jumptable_section")));

/* @} */

/*
 * Initialization of the ker_table for the SFI mode
 */
#define SOS_SFI_KER_TABLE {			\
    SFI_KER_TABLE_ENTRY(0),			\
      SFI_KER_TABLE_ENTRY(1),			\
      SFI_KER_TABLE_ENTRY(2),			\
      SFI_KER_TABLE_ENTRY(3),			\
      SFI_KER_TABLE_ENTRY(4),			\
      SFI_KER_TABLE_ENTRY(5),			\
      SFI_KER_TABLE_ENTRY(6),			\
      SFI_KER_TABLE_ENTRY(7),			\
      SFI_KER_TABLE_ENTRY(8),			\
      SFI_KER_TABLE_ENTRY(9),			\
      SFI_KER_TABLE_ENTRY(10),			\
      SFI_KER_TABLE_ENTRY(11),			\
      SFI_KER_TABLE_ENTRY(12),			\
      SFI_KER_TABLE_ENTRY(13),			\
      SFI_KER_TABLE_ENTRY(14),			\
      SFI_KER_TABLE_ENTRY(15),			\
      SFI_KER_TABLE_ENTRY(16),			\
      SFI_KER_TABLE_ENTRY(17),			\
      SFI_KER_TABLE_ENTRY(18),			\
      SFI_KER_TABLE_ENTRY(19),			\
      SFI_KER_TABLE_ENTRY(20),			\
      SFI_KER_TABLE_ENTRY(21),			\
      SFI_KER_TABLE_ENTRY(22),			\
      SFI_KER_TABLE_ENTRY(23),			\
      SFI_KER_TABLE_ENTRY(24),			\
      SFI_KER_TABLE_ENTRY(25),			\
      SFI_KER_TABLE_ENTRY(26),			\
      SFI_KER_TABLE_ENTRY(27),			\
      SFI_KER_TABLE_ENTRY(28),			\
      SFI_KER_TABLE_ENTRY(29),			\
      SFI_KER_TABLE_ENTRY(30),			\
      SFI_KER_TABLE_ENTRY(31),			\
      SFI_KER_TABLE_ENTRY(32),			\
      SFI_KER_TABLE_ENTRY(33),			\
      SFI_KER_TABLE_ENTRY(34),			\
      SFI_KER_TABLE_ENTRY(35),			\
      SFI_KER_TABLE_ENTRY(36),			\
      SFI_KER_TABLE_ENTRY(37),			\
      SFI_KER_TABLE_ENTRY(38),			\
      SFI_KER_TABLE_ENTRY(39),			\
      SFI_KER_TABLE_ENTRY(40),			\
      SFI_KER_TABLE_ENTRY(41),			\
      SFI_KER_TABLE_ENTRY(42),			\
      SFI_KER_TABLE_ENTRY(43),			\
      SFI_KER_TABLE_ENTRY(44),			\
      SFI_KER_TABLE_ENTRY(45),			\
      SFI_KER_TABLE_ENTRY(46),			\
      SFI_KER_TABLE_ENTRY(47),			\
      SFI_KER_TABLE_ENTRY(48),			\
      SFI_KER_TABLE_ENTRY(49),			\
      SFI_KER_TABLE_ENTRY(50),			\
      SFI_KER_TABLE_ENTRY(51),			\
      SFI_KER_TABLE_ENTRY(52),			\
      SFI_KER_TABLE_ENTRY(53),			\
      SFI_KER_TABLE_ENTRY(54),			\
      SFI_KER_TABLE_ENTRY(55),			\
      SFI_KER_TABLE_ENTRY(56),			\
      SFI_KER_TABLE_ENTRY(57),			\
      SFI_KER_TABLE_ENTRY(58),			\
      SFI_KER_TABLE_ENTRY(59),			\
      SFI_KER_TABLE_ENTRY(60),			\
      SFI_KER_TABLE_ENTRY(61),			\
      SFI_KER_TABLE_ENTRY(62),			\
      SFI_KER_TABLE_ENTRY(63),			\
      SFI_KER_TABLE_ENTRY(64),			\
      SFI_KER_TABLE_ENTRY(65),			\
      SFI_KER_TABLE_ENTRY(66),			\
      SFI_KER_TABLE_ENTRY(67),			\
      SFI_KER_TABLE_ENTRY(68),			\
      SFI_KER_TABLE_ENTRY(69),			\
      SFI_KER_TABLE_ENTRY(70),			\
      SFI_KER_TABLE_ENTRY(71),			\
      SFI_KER_TABLE_ENTRY(72),			\
      SFI_KER_TABLE_ENTRY(73),			\
      SFI_KER_TABLE_ENTRY(74),			\
      SFI_KER_TABLE_ENTRY(75),			\
      SFI_KER_TABLE_ENTRY(76),			\
      SFI_KER_TABLE_ENTRY(77),			\
      SFI_KER_TABLE_ENTRY(78),			\
      SFI_KER_TABLE_ENTRY(79),			\
      SFI_KER_TABLE_ENTRY(80),			\
      SFI_KER_TABLE_ENTRY(81),			\
      SFI_KER_TABLE_ENTRY(82),			\
      SFI_KER_TABLE_ENTRY(83),			\
      SFI_KER_TABLE_ENTRY(84),			\
      SFI_KER_TABLE_ENTRY(85),			\
      SFI_KER_TABLE_ENTRY(86),			\
      SFI_KER_TABLE_ENTRY(87),			\
      SFI_KER_TABLE_ENTRY(88),			\
      SFI_KER_TABLE_ENTRY(89),			\
      SFI_KER_TABLE_ENTRY(90),			\
      SFI_KER_TABLE_ENTRY(91),			\
      SFI_KER_TABLE_ENTRY(92),			\
      SFI_KER_TABLE_ENTRY(93),			\
      SFI_KER_TABLE_ENTRY(94),			\
      SFI_KER_TABLE_ENTRY(95),			\
      SFI_KER_TABLE_ENTRY(96),			\
      SFI_KER_TABLE_ENTRY(97),			\
      SFI_KER_TABLE_ENTRY(98),			\
      SFI_KER_TABLE_ENTRY(99),			\
      SFI_KER_TABLE_ENTRY(100),			\
      SFI_KER_TABLE_ENTRY(101),			\
      SFI_KER_TABLE_ENTRY(102),			\
      SFI_KER_TABLE_ENTRY(103),			\
      SFI_KER_TABLE_ENTRY(104),			\
      SFI_KER_TABLE_ENTRY(105),			\
      SFI_KER_TABLE_ENTRY(106),			\
      SFI_KER_TABLE_ENTRY(107),			\
      SFI_KER_TABLE_ENTRY(108),			\
      SFI_KER_TABLE_ENTRY(109),			\
      SFI_KER_TABLE_ENTRY(110),			\
      SFI_KER_TABLE_ENTRY(111),			\
      SFI_KER_TABLE_ENTRY(112),			\
      SFI_KER_TABLE_ENTRY(113),			\
      SFI_KER_TABLE_ENTRY(114),			\
      SFI_KER_TABLE_ENTRY(115),			\
      SFI_KER_TABLE_ENTRY(116),			\
      SFI_KER_TABLE_ENTRY(117),			\
      SFI_KER_TABLE_ENTRY(118),			\
      SFI_KER_TABLE_ENTRY(119),			\
      SFI_KER_TABLE_ENTRY(120),			\
      SFI_KER_TABLE_ENTRY(121),			\
      SFI_KER_TABLE_ENTRY(122),			\
      SFI_KER_TABLE_ENTRY(123),			\
      SFI_KER_TABLE_ENTRY(124),			\
      SFI_KER_TABLE_ENTRY(125),			\
      SFI_KER_TABLE_ENTRY(126),			\
      SFI_KER_TABLE_ENTRY(127),			\
      }


#endif// _SFI_JUMPTABLE_H_
