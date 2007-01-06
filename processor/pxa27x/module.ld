OUTPUT_FORMAT("elf32-avr","elf32-avr","elf32-avr")
OUTPUT_ARCH(avr:5)
MEMORY
{
  text   (rx)   : ORIGIN = 0x00000000, LENGTH = 64M
  data   (rw!x) : ORIGIN = 0x5c000000, LENGTH = 256K
}
SECTIONS
{
  /* Internal text space or external memory */
  .text :
  {
    *(.progmem*)
    . = ALIGN(2);
    *(.text)
    . = ALIGN(2);
    *(.text.*)
    . = ALIGN(2);
    *(.module)
    . = ALIGN(2);
     _etext = . ;
  }  > text
  .nouse  : AT (ADDR (.text) + SIZEOF (.text))
  {
    *(.vectors)
     __ctors_start = . ;
     *(.ctors)
     __ctors_end = . ;
     __dtors_start = . ;
     *(.dtors)
     __dtors_end = . ;
    *(.progmem.gcc*)
    *(.init0)  /* Start here after reset.  */
    *(.init1)
    *(.init2)  /* Clear __zero_reg__, set up stack pointer.  */
    *(.init3)
    *(.init4)  /* Initialize data and BSS.  */
    *(.init5)
    *(.init6)  /* C++ constructors.  */
    *(.init7)
    *(.init8)
    *(.init9)  /* Call main().  */
    *(.fini9)  /* _exit() starts here.  */
    *(.fini8)
    *(.fini7)
    *(.fini6)  /* C++ destructors.  */
    *(.fini5)
    *(.fini4)
    *(.fini3)
    *(.fini2)
    *(.fini1)
    *(.fini0)  /* Infinite loop after program termination.  */
  }  > text
  .data	  : AT (ADDR (.nouse) + SIZEOF (.nouse))
  {
     PROVIDE (__data_start = .) ;
    *(.data)
    *(.gnu.linkonce.d*)
    . = ALIGN(2);
     _edata = . ;
     PROVIDE (__data_end = .) ;
  }  > data
  .bss  SIZEOF(.data) + ADDR(.data) :
  {
     PROVIDE (__bss_start = .) ;
    *(.bss)
    *(COMMON)
     PROVIDE (__bss_end = .) ;
  }  > data
   __data_load_start = LOADADDR(.data);
   __data_load_end = __data_load_start + SIZEOF(.data);
   /* No debugging sections are included for the Module! */
}
