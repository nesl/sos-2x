/********************************************************************************
*                                                                              *
*    Copyright (C) 2002 Oki Electric Industry Co., LTD.                        *
*                                                                              *
*    System Name    :  ML674000                                                *
*    Module Name    :  assembler common definitions                            *
*    File   Name    :  define.s                                                *
*    Revision       :  01.00                                                   *
*    Date           :  2002/01/29 initial version                              *
*                      2002/03/13 add USR_SWI_MAX                              *
*                      2003/09/04 Ashling Ported to GNU AS                     *
*******************************************************************************/

.global GPCTL
.global Mode_USR
.global Mode_IRQ
.global Mode_SVC
.global Mode_SYS
.global I_Bit  
.global F_Bit  
.global SWI_IRQ_EN
.global SWI_IRQ_DIS
.global USR_SWI_MAX

/* now some standard definitions... */
.equ Mode_USR,       0x10
.equ Mode_IRQ,       0x12
.equ Mode_SVC,       0x13
.equ Mode_SYS,       0x1F
        
.equ I_Bit,          0x80
.equ F_Bit,          0x40

.equ SWI_IRQ_EN,     0x00         /* SWI number of irq_en    */
.equ SWI_IRQ_DIS,    0x01         /* SWI number of irq_dis   */
.equ USR_SWI_MAX,    0x01	  /* Maximum user SWI number */
                                                               
.equ GPCTL,          0xb7000000   /* Address of GPCTL        */

