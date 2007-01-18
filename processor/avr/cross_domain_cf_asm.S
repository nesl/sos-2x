/**
 * \file cross_domain_cf_asm.S
 * \brief Routines for cross domain control flow checks
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <sfi_exception.h>

/*********************************************************************************************
 * CONSTANTS
 *********************************************************************************************/	
#ifdef MICA2_PLATFORM	
//#define CODEMEM_START_PAGE 192 // Ram - Should be even numbered
#define CODEMEM_END_PAGE   469 // Ram - Should be odd numbered
#endif
#ifdef MICAZ_PLATFORM
//#define CODEMEM_START_PAGE 192 // Ram - Should be even numbered
#define CODEMEM_END_PAGE   469 // Ram - Should be odd numbered
#endif
#define KERNEL_DOM_ID  7	
#define WORDS_PER_FLASH_PAGE 128	
#define CODEMEM_CONST_STORE_PAGES 32
#define SFI_KERTBL_START_PAGE (CODEMEM_END_PAGE + 1)  // Ram - Should be an even numbered page. Should be greater than 256
#define SFI_MODTBL_START_PAGE (CODEMEM_END_PAGE + 3)  // Ram - Should be an even numbered page. Should be greater than 256
#define SFI_UP_BND_PAGE       (CODEMEM_END_PAGE + 11) // Ram - Should be an even numbered page. Should be greater than 256 	
//Note: All pages greater than or equal to SFI_UP_BND_PAGE page are not allowed

//CODEMEM_START_WORDH = ((CODEMEM_START_PAGE + CODEMEM_CONST_STORE_PAGES) * WORDS_PER_FLASH_PAGE)/256
//CODEMEM_START_WORDL = ((CODEMEM_START_PAGE + CODEMEM_CONST_STORE_PAGES) * WORDS_PER_FLASH_PAGE) - (256 * CODEMEM_START_WORDH)
CODEMEM_END_WORDH = (CODEMEM_END_PAGE * WORDS_PER_FLASH_PAGE)/256
CODEMEM_END_WORDL = (CODEMEM_END_PAGE * WORDS_PER_FLASH_PAGE) - (256 * CODEMEM_END_WORDH)		
SFI_KERTBL_START_WORDH = (SFI_KERTBL_START_PAGE * WORDS_PER_FLASH_PAGE)/256
SFI_KERTBL_START_WORDL = (SFI_KERTBL_START_PAGE * WORDS_PER_FLASH_PAGE) - (256 * SFI_KERTBL_START_WORDH)
SFI_MODTBL_START_WORDH = (SFI_MODTBL_START_PAGE * WORDS_PER_FLASH_PAGE)/256
SFI_MODTBL_START_WORDL = (SFI_MODTBL_START_PAGE * WORDS_PER_FLASH_PAGE) - (256 * SFI_MODTBL_START_WORDH)
SFI_UP_BND_WORDH       = (SFI_UP_BND_PAGE       * WORDS_PER_FLASH_PAGE)/256
SFI_UP_BND_WORDL       = (SFI_UP_BND_PAGE       * WORDS_PER_FLASH_PAGE) - (256 * SFI_UP_BND_WORDH)
	
MAX_MOD_DOMS           = 7
	
__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d	
xptr_l = 26
xptr_h = 27
zptr_l = 30
zptr_h = 31
word_l = 20	
word_h = 21
temp_reg0 = 0
temp_reg1 = 1
param0 = 24
param1 = 25	

/*********************************************************************************************
 * TEXT SECTION
 *********************************************************************************************/
.section .text
	
;;; Brief: Cross domain call for scheduling the message handler
;;; Prototype: int8_t ker_cross_domain_call_mod_handler(void* state, Message* m, msg_handler_t handler);
;;; Description: If module handler resides in SFI Module Jumptable, then its a cross domain call.
;;;              Else a module handler within the kernel is being scheduled.
.global ker_cross_domain_call_mod_handler
ker_cross_domain_call_mod_handler:
	movw zptr_l, word_l
	ldi xptr_h, SFI_MODTBL_START_WORDH
	cp  zptr_h, xptr_h
	brcc ker_cross_domain_call
	ijmp


;;; Brief: Checks icalls originating from the kernel
;;; Just check for a cross domain call
.global ker_icall_from_kernel_check
ker_icall_from_kernel_check:
#ifdef SFI_DOMS_2
	ldi xptr_h, SFI_UP_BND_WORDH
	cp zptr_h, xptr_h
	brcc label_ker_icall_check_fail
#endif	
	ldi xptr_h, SFI_MODTBL_START_WORDH
	cp  zptr_h, xptr_h
	brcc ker_cross_domain_call
	ijmp
	
;;; Brief: Checks icall target address
;;; Description: icall target address always resides in register pair zptr_h:zptr_l
;;; If call target is in SFI Module Jumptable, its a cross domain call
;;; If call target is not in SFI Kernel Jumptable, its an exception
;;; Assumption: SFI_KERTBL_START_PAGE and SFI_MODTBL_START_PAGE are even
;;;             This assumption optimizes checks as only top 8 address bits have to be compared
.global ker_icall_check
ker_icall_check:
#ifdef SFI_DOMS_2
	ldi xptr_h, SFI_UP_BND_WORDH
	cp zptr_h, xptr_h
	brcc label_ker_icall_check_fail
#endif	
	ldi xptr_h, SFI_MODTBL_START_WORDH
	cp  zptr_h, xptr_h
	brcc ker_cross_domain_call
	ldi xptr_h, SFI_KERTBL_START_WORDH
	cp zptr_h, xptr_h
	brcs label_ker_icall_check_fail
	ijmp
label_ker_icall_check_fail:
	eor param1, param1
	ldi param0, KER_ICALL_CHECK_EXCEPTION
	call sfi_exception
	

;;; Brief: Cross Domain Call Stub
;;; Description:
.global ker_cross_domain_call
ker_cross_domain_call:
	push zptr_l					; Stack: zptr_l:retH:retL
	push zptr_h					; Stack: zptr_h:zptr_l:retH:retL
	in temp_reg0, __SREG__				; temp_reg0 = SREG
	cli						; Disable Interrupts
	in zptr_l, __SP_L__				; zptr_l = SP_L
	in zptr_h, __SP_H__				; zptr_h = SP_H
	lds xptr_l, (safe_stack_ptr)			; xptr_l = safe_stack_ptr_l
	lds xptr_h, (safe_stack_ptr+1)			; xptr_h = safe_stack_ptr_h
	ldd temp_reg1, Z+4				; temp_reg1 = retL
	st X+, temp_reg1				; SafeStack: retL
	ldd temp_reg1, Z+3				; temp_reg1 = retH
	st X+, temp_reg1				; SafeStack: retH:retL
	lds temp_reg1, domain_stack_bound		; temp_reg1 = dsbL
	st X+, temp_reg1				; SafeStack: dsbL:retH:retL
	lds temp_reg1, (domain_stack_bound+1)		; temp_reg1 = dsbH
	st X+, temp_reg1				; SafeStack: dsbH:dsbL:retH:retL
#ifdef SFI_DOMS_8	
	lds temp_reg1, curr_dom_id			; temp_reg1 = domID
	st X+, temp_reg1				; SafeStack: domID:dsbH:dsbL:retH:retL
#endif	
	adiw zptr_l, 3					; Z = addr(retH) = dsb
	sts domain_stack_bound, zptr_l			; dsbL = zptr_l
	sts (domain_stack_bound + 1), zptr_h		; dsbH = zptr_h
	ldi zptr_l, lo8(pm(ker_cross_domain_return))	; zptr_l = cdretL
	st X+, zptr_l					; SafeStack: cdretL:domID:dsbH:dsbL:retH:retL
	ldi zptr_l, hi8(pm(ker_cross_domain_return))	; zptr_l = cdretH
	st X+, zptr_l					; SafeStack: cdretH:cdretL:domID:dsbH:dsbL:retH:retL
	sts safe_stack_ptr, xptr_l			; safe_stack_ptr_l = xptr_l
	sts (safe_stack_ptr+1), xptr_h			; safe_stack_ptr_h = xptr_h
	pop zptr_h					; Stack: (zptr_h):zptr_l:retH:retL
	pop zptr_l					; Stack: (zptr_l):retH:retL
#ifdef SFI_DOMS_8	
	movw xptr_l, zptr_l				; xptr_h:xptr_l = zptr_h:zptr_l
	lsl xptr_l					; 
	rol xptr_h					; xptr_h has (pageNum - 256)
	ldi xptr_l, SFI_MODTBL_START_WORDH		; 
	lsl xptr_l					; xptr_l has (SFI_MODTBL - 256)
	sub xptr_h, xptr_l				; domID = pageNum - SFIMODTBL
	sts curr_dom_id, xptr_h 			; Store domID
	ldi xptr_l, MAX_MOD_DOMS			; 
	cp xptr_h, xptr_l				; if (domID >= MAX_MOD_DOMS)
	brcc label_ker_icall_check_fail			; then goto label_ker_icall_check_fail
#endif	
	clr temp_reg1					; temp_reg1 = 0
	out __SREG__, temp_reg0				; restore SREG
	ijmp						; Indirect jump into jump table
ker_cross_domain_return:
	in temp_reg0, __SREG__				; temp_reg0 = SREG
	cli						; Disable Interrupts
	lds xptr_l, (safe_stack_ptr)			; xptr_l = safe_stack_ptr_l
	lds xptr_h, (safe_stack_ptr+1)			; xptr_h = safe_stack_ptr_h
#ifdef SFI_DOMS_8		
	ld temp_reg1, -X				; SafeStack:(domID):dsbH:dsbL:retH:retL
	sts curr_dom_id, temp_reg1			; Restore previous domID
#endif
	ld temp_reg1, -X				; SafeStack: (dsbH):dsbL:retH:retL
	sts (domain_stack_bound+1),temp_reg1		; Restore dsbH
	ld temp_reg1, -X				; SafeStack: (dsbL):retH:retL
	sts (domain_stack_bound), temp_reg1		; Restore dsbL
	ld zptr_h, -X					; SafeStack: (retH):retL
	ld zptr_l, -X					; SafeStack: (retL)
	sts safe_stack_ptr, xptr_l			; Store safe_stack_ptr_l 
	sts (safe_stack_ptr+1), xptr_h			; Store safe_satck_ptr_h
	out __SREG__, temp_reg0				; Restore SREG
	clr temp_reg1					; Clear temp_reg1
	ijmp						; Indirect jump to return address


.global ker_restore_ret_addr
ker_restore_ret_addr:	
	push xptr_l				; Stack: xptr_l:retH:retL
	push xptr_h				; Stack: xptr_h:xptr_l:retH:retL
	push zptr_l				; Stack: zptr_l:xptr_h:xptr_l:retH:retL
	push zptr_h				; Stack: zptr_h:zptr_l:xptr_h:xptr_l:retH:retL
	in temp_reg0, __SREG__			; temp_reg0 = SREG
	cli					; Disable Interrupts
	lds xptr_l, safe_stack_ptr		; xptr_l = safe_stack_ptr_l
	lds xptr_h, (safe_stack_ptr+1)		; xptr_h = safe_stack_ptr_h
	in zptr_l, __SP_L__			; zptr_l = SP_L
	in zptr_h, __SP_H__			; zptr_h = SP_H
	ld temp_reg1, -X			; SafeStack: (retSH):retSL
	std Z+5, temp_reg1			; Stack: zptr_h:zptr_l:xptr_h:xptr_l:{retSH}:retL
	ld temp_reg1,-X				; SafeStack: (retSL)
	std Z+6, temp_reg1			; Stack: zptr_h:zptr_l:xptr_h:xptr_l:retSH:{retSL}
	sts safe_stack_ptr, xptr_l		; xptr_l = safe_stack_ptr_l
	sts (safe_stack_ptr+1), xptr_h		; xptr_h = safe_stack_ptr_h
	out __SREG__, temp_reg0			; Restore SREG
	clr temp_reg1				; Clear temp_reg1
	pop zptr_h				; Stack: (zptr_h):zptr_l:xptr_h:xptr_l:retSH:retSL
	pop zptr_l				; Stack: (zptr_l):xptr_h:xptr_l:retSH:retSL
	pop xptr_h				; Stack: (xptr_h):xptr_l:retSH:retSL
	pop xptr_l				; Stack: (xptr_l):retSH:retSL
	ret					; return to saved address
	
.global ker_save_ret_addr
ker_save_ret_addr:
	push xptr_l				; Stack: xptr_l:retH:retL:retSH:retSL
	push xptr_h				; Stack: xptr_h:xptr_l:retH:retL:retSH:retSL
	push zptr_l				; Stack: zptr_l:xptr_h:xptr_l:retH:retL
	push zptr_h				; Stack: zptr_h:zptr_l:xptr_h:xptr_l:retH:retL:retSH:retSL
	in temp_reg0, __SREG__			; temp_reg0 = SREG
	cli					; Disable Interrupts
	lds xptr_l, safe_stack_ptr		; xptr_l = safe_stack_ptr_l
	lds xptr_h, (safe_stack_ptr+1)		; xptr_h = safe_stack_ptr_h
	in zptr_l, __SP_L__			; zptr_l = SP_L
	in zptr_h, __SP_H__			; zptr_h = SP_H
	ldd temp_reg1, Z+8			; temp_reg1 = retSL
	st X+, temp_reg1			; SafeStack: retSL
	ldd temp_reg1, Z+7			; temp_reg1 = retSH
	st X+, temp_reg1			; SafeStack: retSH:retSL
	sts safe_stack_ptr, xptr_l		; safe_stack_ptr_l = xptr_l
	sts (safe_stack_ptr+1), xptr_h		; safe_stack_ptr_h = xptr_h
	out __SREG__,temp_reg0			; Restore SREG
	pop zptr_h				; Stack: (zptr_h):zptr_l:xptr_h:xptr_l:retH:retL:retSH:retSL
	pop zptr_l				; Stack: (zptr_l):xptr_h:xptr_l:retH:retL:retSH:retSL
	pop xptr_h				; Stack: (xptr_h):xptr_l:retH:retL:retSH:retSL
	pop xptr_l				; Stack: (xptr_l):retH:retL:retSH:retSL
	clr temp_reg1				; Clear temp_reg1
	ret					; Return to call
	
	
	
;;; End of file
