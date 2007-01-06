/*******************************************************************************
*                                                                              *
*    Copyright (C) 2002 Oki Electric Industry Co., LTD.                        *
*                                                                              *
*    System Name    :  ML674000                                                *
*    Module Name    :  Startup routine                                         *
*    File   Name    :  init.s                                                  *
*    Revision       :  01.00                                                   *
*    Date           :  2001/12/20  initial version                             *
*                      2003/09/04 Ashling Ported to GNU AS                     *
*                                                                              *
*******************************************************************************/
.include "define.s"

        
/* locations of various things in our memory system */
.equ RAM_Base,        0x50000000              /* bottom of internal RAM        */
.equ RAM_Size,        0x8000                  /* size of internal RAM : 8kbyte */
.equ RAM_Limit,       RAM_Base+RAM_Size       /* top of internal RAM           */
                                                
.equ IRQ_Stack,       RAM_Limit               /* top of IRQ stack              */
.equ SVC_Stack,       RAM_Limit-1024          /* top of SVC stack              */
.equ USR_Stack,       SVC_Stack-1024          /* top of USR stack              */

/* --- setup interrupt / exception vectors*/
        B       Reset_Handler
        B       Undefined_Handler
        B       SWI_Handler
        B       Prefetch_Handler
        B       Abort_Handler
        NOP                                     /* reserved vector */
        B       IRQ_Handler
        B       FIQ_Handler

/****************************************
*      Undefined Handler                *
*****************************************/
Undefined_Handler:
        B       Undefined_Handler
        
/*******************************
*      SWI Handler             *
*******************************/
SWI_Handler:
        STMFD   sp!, {r1-r12,lr}                /* save registers */
        LDR     r1, [lr,#-4]                    /* get instruction code */
        BIC     r1, r1, #0xffffff00             /* decode SWI number */
        BIC     r2, r1, #0x0000000f             /* decode SWI_Jump_Table number */
        AND     r1, r1, #0x0000000f            
        ADR     r3, SWI_Jump_Table_Table        /* get address of jump table table */
        LDR     r4, [r3, r2, LSR #2]            /* get address of jump table */
        LDR     pc, [r4,r1,LSL #2]              /* refer to jump table and branch. */
SWI_Jump_Table_Table:                          
        .long     SWI_Jump_Table               
        .long     SWI_PM_Jump_Table            
SWI_Jump_Table:                                /* normal SWI jump table */
        .long     SWI_irq_en                   
        .long     SWI_irq_dis                  
SWI_irq_en:                                     /* enable IRQ */
        MRS     r0, SPSR                        /* get SPSR */
        BIC     r3, r0, #I_Bit                  /* I_Bit clear */
        AND     r0, r0, #I_Bit                  /* return value */
        MSR     SPSR_c, r3                      /* set SPSR */
        B       EndofSWI                       
SWI_irq_dis:                                    /* disable IRQ*/
        MRS     r0, SPSR                        /* get SPSR */
        ORR     r3, r0, #I_Bit                  /* I_Bit set */
        AND     r0, r0, #I_Bit                  /* return value */
        MSR     SPSR_c, r3                      /* set SPSR */
        B       EndofSWI
SWI_PM_Jump_Table:                       /* SWI jump table for power m  anagement */
        .long     SWI_pm_if_recov
        .long     SWI_pm_if_dis
SWI_pm_if_recov:                         /* recover CPSR */
        MSR     SPSR_c, r0                      /* Set SPSR */
        B       EndofSWI
SWI_pm_if_dis:                           /* mask IRQ and FIQ. and returnpre-masked CPSR */
        MRS     r0, SPSR                        /* Get SPSR & Return value */
        ORR     r3, r0, #I_Bit
        MSR     SPSR_c, r3                      /* Set SPSR */
        B       EndofSWI
EndofSWI:
        LDMFD   sp!, {r1-r12,pc}^               /* restore registers */
        
/***************************************
*      Prefetch Handler                *
***************************************/
Prefetch_Handler:
        B       Prefetch_Handler
        
/*******************************
*      Abort Handler           *
*******************************/
Abort_Handler:
        B       Abort_Handler
        
/*******************************
*      FIQ Handler             *
*******************************/
FIQ_Handler:
        B       FIQ_Handler

/*******************************
*      Reset Handler           *
*******************************/
/* the RESET entry point */
Reset_Handler:

/* --- initialize stack pointer registers */
/* enter IRQ mode and set up the IRQ stack pointer */
        MOV     R0, #Mode_IRQ|I_Bit|F_Bit       /* no interrupts */
        MSR     CPSR_c, R0
        LDR     R13, =IRQ_Stack                 /* set IRQ mode stack. */

/* set up the SVC stack pointer last and return to SVC mode */
        MOV     R0, #Mode_SVC|I_Bit|F_Bit       /* no interrupts */
        MSR     CPSR_c, R0
        LDR     R13, =SVC_Stack                 /* set SVC mode STACK. */
        
/* --- initialize memory required by C code */
    
    /* Clear uninitialized data section (bss) */
    ldr     r4, =__start_bss      /* First address*/      
    ldr     r5, =__end_bss       /* Last  address*/      
    mov     r6, #0x0                                
                                                
loop_zero:  
    str     r6, [r4]                                
    add     r4, r4, #0x4                            
    cmp     r4, r5                                  
    blt     loop_zero

    /* Copy initialized data sections into RAM */
    ldr     r4, =_fdata      /* destination address */      
    ldr     r5, =_edata      /* Last  address*/      
    ldr     r6, =_etext      /* source address*/      
    cmp     r4, r5                                  
    beq     skip_initialize

loop_initialise:
    ldr     r3, [r6]
    str     r3, [r4]                                
    add     r4, r4, #0x4
    add     r6, r6, #0x4
    cmp     r4, r5                                  
    blt     loop_initialise
    
skip_initialize:
        
/* --- now change to user mode and set up user mode stack. */
        MOV     R0, #Mode_USR|F_Bit|I_Bit
        MSR     CPSR_c, R0
        LDR     sp, =USR_Stack                  /* set USR mode stack. */

/* --- now enable external bus function(second function of PIOC[2:6]). */
/*
        LDR     R0, =GPCTL
        LDRH    R1, [R0]
        ORR     R1, R1, #0x4
        STRH    R1, [R0]
*/

/* --- now enter the C code */
        BL      main       /* branch to main function */
        
END_LOOP:
        B       END_LOOP



