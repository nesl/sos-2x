/**********************************************************************************/
/*                                                                                */
/*    Copyright (C) 2001 Oki Electric Industry Co., LTD.                          */
/*                                                                                */
/*    System Name    :  uPLAT series                                              */
/*    Module Name    :  header file for using IRQ                                 */
/*    File   Name    :  irq.h                                                     */
/*    Revision       :  01.00                                                     */
/*    Date           :  2001/12/20 initial version                                */
/*                                                                                */
/**********************************************************************************/
#ifndef IRQ_H
#define IRQ_H

#include <hardware_proc.h>
#include <sos_inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* type definition */
typedef void IRQ_HANDLER(void);
typedef IRQ_HANDLER *pIRQ_HANDLER;

/* functions */

/**********************************************************************/
/*  Enable IRQ                                                        */
/*  Function : UWORD irq_en(void)                                     */
/*      Parameters                                                    */
/*          input  : nothing                                          */
/*          output : IRQ state before change                          */
/*                   0 : Enable                                       */
/*                   others : Disable                                 */
/**********************************************************************/
uint32_t irq_en(void);

/**********************************************************************/
/*  Disable IRQ                                                       */
/*  Function : UWORD irq_dis(void)                                    */
/*      Parameters                                                    */
/*          input  : nothing                                          */
/*          output : IRQ state before change                          */
/*                   0 : Enable                                       */
/*                   others : Disable                                 */
/**********************************************************************/
uint32_t irq_dis(void);

/**********************************************************************/
/*  Get IRQ State                                                     */
/*  Function : UWORD get_irq_state(void)                              */
/*      Parameters                                                    */
/*          input  : nothing                                          */
/*          output : IRQ state                                        */
/*                   0 : Enable                                       */
/*                   others : Disable                                 */
/**********************************************************************/
uint32_t get_irq_state(void);

/**********************************************************************/
/*  Set IRQ State                                                     */
/*  Function : UWORD set_irq_state(void)                              */
/*      Parameters                                                    */
/*          input  : IRQ State to be set                              */
/*                   0 : Enable                                       */
/*                   others : Disable                                 */
/*          output : IRQ state before change                          */
/*                   0 : Enable                                       */
/*                   others : Disable                                 */
/**********************************************************************/
#define set_irq_state(n)    (n == 0 ? irq_en() : irq_dis())

/**********************************************************************/
/*  Initialize Interrupt Control Registers (IRQ interrupt)            */
/*  Function : init_irq                                               */
/*      Parameters                                                    */
/*          Input   :   Nothing                                       */
/*          Output  :   Nothing                                       */
/*  Note : This function initializes only IRQ interrupt,              */
/*         and doesn't initialize FIQ and expanded interrupt.         */
/**********************************************************************/
void init_irq(void);

#ifndef IRQSIZE
#define IRQSIZE 32
#endif

#if IRQSIZE < 16
#error IRQSIZE needs to be 16 or more.
#endif

/**********************************************************************/
/*  Table of IRQ handler                                              */
/*      If interrupt of interrupt number N occurred,                  */
/*      function of IRQ_HANDLER_TABLE[N] is called.                   */
/**********************************************************************/
extern pIRQ_HANDLER IRQ_HANDLER_TABLE[];

#ifdef __cplusplus
};      /* end of 'extern "C"' */
#endif
#endif  /* #define IRQ_H */
