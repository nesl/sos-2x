#ifndef CPLDCONST_H
#define CPLDCONST_H


//states
enum { 
  CPLD_STANDBY=0x35             ,     // standby
  CPLD_RUN_CAMERA               ,     // run camera
  CPLD_CAPTURE_IMAGE            ,     // capture image
  CPLD_CAPTURE_TEST_PATTERN     ,     // generate test image
  CPLD_MCU_ACCESS_SRAM          ,     // direct uP to SRAM access
  CPLD_MCU_ACCESS_FLASH         ,     // direct uP to Flash access
  CPLD_TRANSFER_SRAM_TO_FLASH   ,     // transfer SRAM to Flash
  CPLD_TRANSFER_FLASH_TO_SRAM   ,     // transfer Flash to SRAM
	CPLD_MCU_ACCESS_SRAM_TEST, 
  CPLD_OFF
};

//CPLD opCode
//Defined by rick according to implementation of CPLD.
enum { 
  CPLD_OPCODE_STANDBY                  =0x0,     // standby
  CPLD_OPCODE_RESET                    =0x1,     // reset
  CPLD_OPCODE_RUN_CAMERA               =0x2,     // run camera
  CPLD_OPCODE_CAPTURE_IMAGE            =0x3,     // capture image
  CPLD_OPCODE_CAPTURE_TEST_PATTERN     =0x4,     // generate test image
  CPLD_OPCODE_MCU_ACCESS_SRAM          =0x5,     // direct uP to SRAM access
  CPLD_OPCODE_MCU_ACCESS_FLASH         =0x6,     // direct uP to Flash access
  CPLD_OPCODE_TRANSFER_SRAM_TO_FLASH   =0x7,     // transfer SRAM to Flash
  CPLD_OPCODE_TRANSFER_FLASH_TO_SRAM   =0x9,     // transfer Flash to SRAM
CPLD_OPCODE_MCU_ACCESS_SRAM_TEST = 0xA,	
};

struct cpldCommand_s 
{
  uint8_t opcode;
  uint8_t sramBank:4;
  uint8_t flashBank:4;
  uint8_t startPageAddress;
  uint8_t endPageAddress;
}__attribute__((packed));

typedef struct cpldCommand_s cpldCommand_t;

#endif
