/*******************************************************************************
*                                                                              *
*    Copyright (C) 2002 Oki Electric Industry Co., LTD.                        *
*                                                                              *
*    System Name    :  ML674000                                                *
*    Module Name    :  Reentrant irq handler routine                           *
*    File   Name    :  reentrant_irq_handler.s                                 *
*    Revision       :  01.00                                                   *
*    Date           :  2001/12/20 initial version                              *
*                      2003/09/04 Ashling Ported to GNU AS                     *
*                                                                              *
*******************************************************************************/


/*******************************************************************************
*   <<< bit field of status registers (CPSR, SPSR) >>>                         *
*   31  30  29  28        7   6   5   4   3   2   1   0                        *
* +---+---+---+---+-----+---+---+---+---+---+---+---+---+                      *
* | N | Z | C | V | - - | I | F | T | M4| M3| M2| M1| M0|                      *
* +---+---+---+---+-----+---+---+---+---+---+---+---+---+                      *
* M0-M4 10010 : IRQ mode                                                       *
*       11111 : SYSTEM mode                                                    *
*   T       0 : ARM mode                                                       *
*           1 : THUMB mode                                                     *
*   F       0 : FIQ is allowed                                                 *
*           1 : FIQ is not allowed                                             *
*   I       0 : IRQ is allowed                                                 *
*           1 : IRQ is not allowed                                             *
* N,Z,C,V     : condition flags. flags change with the results of ALU.         *
*                                                                              *
*                                                                              *
*   <<< use situation of registers >>>                                         *
*        IRQ         change to       handler   change to  IRQ                  *
*       start         SYS mode     start  end  IRQ mode   end                  *
*       --|--------------|-----------|-----|-----|--------|-->                 *
*      r0 +--+--+--@========@========@--X--+--+--+--+--+--+ r0                 *
*      r1 +--+--+--+--+--+--+--@=====@--X--+--+--+--+--+--+ r1                 *
*      r2 +--+--+--+--+--+--+--+--+--@--X--+--+--+--+--@--+ r2                 *
*      r3 +--+--+--+--+--@--+--+--+--+--X--+--+--@--+--+--+ r3                 *
*      r4 +--@========@=================O=====@=====@--+--+ r4                 *
*      r5 +--+--@=======================O==============@--+ r5                 *
*  r6-r11 +--+--+--+--+--+--+--+--+--+--O--+--+--+--+--+--+ r6-r11             *
*     r12 +--+--+--+--+--+--+--+--+--+--X--+--+--+--+--+--+ r12                *
*  lr_IRQ @=============== = = = = = = = = = = = =========@ lr_IRQ             *
*  lr_USR - - - - - - - - --+--+--@=====O==@--+-- - - - - - lr_USR             *
*spsr_IRQ @=============== = = = = = = = = = = = =========@ spsr_IRQ           *
*         |<------------>|<--------------------->|<------>|                    *
*             IRQ mode           SYS mode         IRQ mode                     *
*******************************************************************************/

/* common definitions */
.include "define.s"

#define IRQSIZE      32                     /* number of IRQ interrupt factor. */
#define IRQ_BASE     0x78000000             /* base address of registers about IRQ. */

/* definition of alias of registers */
sp_IRQ                  .req    sp          /* r13 */
lr_IRQ                  .req    r14         /* r14 */
sp_USR                  .req    sp          /* r13 */
lr_USR                  .req    r14         /* r14 */
irq_handler_table       .req    r1          /* address of irq_handler_table is saved. */
cil_clear               .req    r2          /* value of this is set to CILCL register. */
address_of_handler      .req    r2          /* address of handler corresponding to  */
                                            /* intrrupt factor is sabed. */
cpsr_tmp                .req    r3          /*  */
saved_spsr_irq          .req    r4          /* value of spsr_irq is saved. */
irq_base                .req    r5          /* base address of registers about IRQ is saved. */
irn                     .req    r6          /* value of IRN register is saved. */


.global irq_en
.global irq_dis

/*********************************************************************
*  IRQ Handler                                                       *
*  Function : void IRQ_Handler(void)                                 *
*      Parameters                                                    *
*          input  : nothing                                          * 
*          output : nothing                                          *
*********************************************************************/
.global     IRQ_Handler
.func       IRQ_Handler

IRQ_Handler:
        SUB     lr_IRQ, lr_IRQ, #4          /* construct the return address */

        /* registers which may be overwritten are saved.(IRQ mode) */
        /* r0-r5 : these are used in this handler. */
        /* r14(r14) : if IRQ handler is reentered, this is overwritten. */
        /* registers which may be overwritten are r1-r6,lr_IRQ(r14). */
        STMFD   sp_IRQ!, {r1-r6, r14}

        /* spsr_IRQ is saved to saved_spsr_irq(r4). */
        /* if IRQ handler is reentered, spsr_IRQ is overwritten. */
        MRS     r4, spsr

        /* IRQ number is got from IRN register. IRQ number is saved to irn(r6). */
        /* after the value of IRN register is read, */
        /* the bit of CIL register corresponding to interrupt level is set. */
        MOV     r5, #0x78000000             /* IRQ_BASE(0x78000000) is saved to irq_base(r5). */
        LDR     irn, [r5, #0x14]            /* IRQ number is saved to irn(r6). */

        /* mode is changed into SYS mode. and IRQ is enabled. */
        /* if IRQ is enabled before a CIL register is set, */
        /* this program does not operate appropriately. */
        /* in SYS mode, USR mode registers are used. */
        TST     r4, #F_Bit                  /* FIQ is available ? */
        MOVEQ   r3, #0x1F                   /* available */
        MOVNE   r3, #0x5f                   /* not abailable */
        MSR     cpsr_c, r3                  /* change to SYS mode and enable IRQ */

        /* check IRQ number */
        /* if IRQ number is invalid(irn > IRQSIZE), */
        /* this routine doesn't branch to handler corresponding to interrupt's factor. */
        CMP     irn, #0x20
        BCS     LABEL

        /* USR mode registers which may be overwritten */
        /* and registers which are not saved by callee are saved. */
        /* -- USR mode registers which may be overwritten -- */
        /* lr_USR(r14) : this is overwritten. */
        /* -- registers which is not saved by callee -- */
        /* r0-r3,r12 : these aren't saved by callee. */
        /*             but there is no influence even if values of r1-r3 change. */
        /* registers which need to be saved are r0, r12 and lr_USR. */
        STMFD   sp!, {r0, r12, lr_USR}      /* R0, R12 and lr_USR(r14) are saved.*/

        /*; address of IRQ_HANDLER_TABLE is got. */
        /*; address of IRQ_HANDLER_TABLE is saved to irq_handler_table(r1). */
        LDR     r1, =IRQ_HANDLER_TABLE

        /*; branch to handler corresponding to interrupt's factor */
        BL      BRANCH_TO_HANDLER

        LDMFD   sp!, {r0, r12, r14}         /* R0, R12 and link register is restored.*/
LABEL:

        /* mode is changed to IRQ mode. and IRQ is disabled. */
        /* if IRQ is still being allowed after CIL register is cleared, */
        /* this program does not operate appropriately. */
        TST     r4, #F_Bit                  /* FIQ is available ? */
        MOVEQ   r3, #0x92                   /* available */
        MOVNE   r3, #0xD2                   /* not abailable */
        MSR     cpsr_c, r3                  /* change to IRQ mode and disable IRQ */
        MSR     spsr_cf, r4                 /* spsr_IRQ is restored. */

        /*; the most significant '1' bit of CIL register is cleared. */
        /*; if arbitrary value is written in CILCL register, */
        /*; the most significant '1' bit of CIL register will be cleared. */
        STR     cil_clear, [r5, #0x28]      /* arbitrary value is written to CILCL register.*/

        /*; saved registers are restored, and control is returned from IRQ. */
        LDMFD   sp!, {r1-r6, pc}^

/* end of IRQ_Handler */
.size   IRQ_Handler,.-IRQ_Handler;
.endfunc
/*********************************************************************
*  Branch to handler corresponding to interrupt's factor.            *
*  Handler doesn't return to this function.                          *
*  Handler directry returns to IRQ_Handler.                          *
*  Function : void BRANCH_TO_HANDLER(void)                           *
*      Parameters                                                    *
*          input  : nothing                                          * 
*          output : nothing(This function doesn't return.)           *
*********************************************************************/
.global BRANCH_TO_HANDLER
.func BRANCH_TO_HANDLER

BRANCH_TO_HANDLER:
        /* address of handler and information that handler is ARM or THUMB */
        /* is saved at irq_handler_table + irn*4. */
        LDR     r2, [r1, r6, lsl #2]; 
        BX      r2                          /* branch to handler corresponding to */
                                            /* interrupt's factor */

/* end of BRANCH_TO_HANDLER */
.size   BRANCH_TO_HANDLER,.-BRANCH_TO_HANDLER;
.endfunc

/*********************************************************************
*  Enable IRQ                                                        *
*  Function : UWORD irq_en(void)                                     *
*      Parameters                                                    *
*          input  : nothing                                          * 
*          output : IRQ state before change                          *
*                   0 : Enable                                       *
*                   others : Disable                                 * 
*********************************************************************/
.global irq_en
.func irq_en

irq_en:
        SWI     0x0
        MOV     pc, lr

/* end of irq_en */
.size   irq_en,.-irq_en;
.endfunc

/*********************************************************************
*  Disable IRQ                                                       *
*  Function : UWORD irq_dis(void)                                    *
*      Parameters                                                    *
*          input  : nothing                                          * 
*          output : IRQ state before change                          *
*                   0 : Enable                                       *
*                   others : Disable                                 * 
*********************************************************************/
.global irq_dis
.func irq_dis

irq_dis:
        SWI     0x01
        MOV     pc, lr
        
/* end of irq_dis */
.size   irq_dis,.-irq_dis;
.endfunc

/*********************************************************************
*  Get IRQ State                                                     *
*  Function : UWORD get_irq_state(void)                              *
*      Parameters                                                    *
*          input  : nothing                                          * 
*          output : IRQ state                                        *
*                   0 : Enable                                       *
*                   others : Disable                                 * 
*********************************************************************/
.global get_irq_state
.func get_irq_state

get_irq_state:
        MRS     r0, CPSR        /* get CPSR */
        AND     r0, r0, #0x80
        MOV     pc, lr

/* end of get_irq_state  */
.size   get_irq_state,.-get_irq_state;
.endfunc

