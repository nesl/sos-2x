/*
   Desc: Assembly routine for fast memmap checking
   Author: Ram Kumar {ram@ee.ucla.edu}
   xptr_h:xptr_l = Write Address	
   temp_reg0 = Data to store	
*/
#include <sfi_exception.h>	
/********************************************
 *          CONSTANTS
 ********************************************/
__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
temp_reg0 = 0   
temp_reg1 = 1   
param0 = 24
param1 = 25
xptr_l = 26	
xptr_h = 27
yptr_l = 28
yptr_h = 29
zptr_l = 30     
zptr_h = 31

/********************************************
 *          DATA SECTION
 ********************************************/
.section .data
.extern safe_store_addr
.extern memmap	
/********************************************
 *          TEXT SECTION
 ********************************************/
.section .text
.extern MEMMAP_PERMS_BM_LUT
.extern BLK_HDR_CHECK_BM_LUT
.global ker_memmap_perms_check_xptr

ker_memmap_perms_check_xptr:
	push temp_reg1          	; STACK: temp_reg1
	in temp_reg1, __SREG__  	; temp_reg1 = SREG
	cli				; Disable interrupts
	push temp_reg1                  ; STACK: SREG|temp_reg1
	push zptr_l			; STACK: zptr_l|SREG|temp_reg1
	push zptr_h			; STACK: zptr_h|zptr_l|SREG|temp_reg1
	push temp_reg0                  ; STACK: temp_reg0|zptr_h|zptr_l|SREG|temp_reg1
	in zptr_l, __SP_L__
	in zptr_h, __SP_H__
	cp xptr_l, zptr_l		; 
	cpc xptr_h, zptr_h		; 
	brcc stack_bound_checker_xptr	; if (X >= Stack Pointer) then do a stack bound check
	movw zptr_l, xptr_l     	; zptr_h:zptr_l = xptr_h:xptr_l
	lsr zptr_h			; if (NUM_DOMAINS == 8) zptr_h:zptr_l >>= 4 ;
	ror zptr_l		 	; if (NUM_DOMAINS == 2) zptr_h:zptr_l >>= 5;
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
#ifdef SFI_DOMS_2
	lsr zptr_h
	ror zptr_l			; zptr_h:zptr_l = memmap_offset
#endif	
	subi zptr_l, lo8(-(memmap))
	sbci zptr_h, hi8(-(memmap))     ; zptr_h:zptr_l = &memmap[memmap_offset]
	ld  temp_reg1,Z                 ; temp_reg1 = perms_byte = memmap[memmap_offset]
	mov zptr_l, xptr_l              ; zptr_l = xptr_l
	eor zptr_h, zptr_h		; clear zptr_h
#ifdef SFI_DOMS_2
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(MEMMAP_PERMS_BM_LUT))
	sbci zptr_h,hi8(-(MEMMAP_PERMS_BM_LUT))	; zptr_h:zptr_l = &MEMMAP_PERMS_BM_LUT[address_offset]
	lpm temp_reg0,Z                 ; temp_reg0 = perms_mask = MEMMAP_PERMS_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = perms_byte & perms_mask
	and temp_reg0, temp_reg0
	brne heap_write_exception_xptr  ; if (temp_reg0 != 0) then goto heap_write_exception_xptr
	eor zptr_h, zptr_h		; clear zptr_h
	mov zptr_l, xptr_l              ; zptr_l = xptr_l
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0,Z			; temp_reg0 = blkdhr_mask = BLK_HDR_CHECK_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = (perms_byte & blkhdr_mask)
	and temp_reg0, temp_reg0
	brne heap_write_exception_xptr  ; if (temp_reg0 != 0) then goto heap_write_exception_xptr
#endif	
#ifdef SFI_DOMS_8
	andi zptr_l, 0x08               ; zptr_l indicates which nibble we are interested in
	cpse zptr_h, zptr_l		; if (zptr_h == zptr_l) i.e. if (0 == zptr_l), skip next instr.
	swap temp_reg1			; Swap nibbles of perms
	mov zptr_l, temp_reg1
	andi zptr_l, 0x07
	lds zptr_h, curr_dom_id
	cp zptr_h, zptr_l
	brne heap_write_exception_xptr
	mov zptr_l, xptr_l
	andi zptr_l, 0x07
	eor zptr_h, zptr_h
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0, Z
	and temp_reg0, temp_reg1
	and temp_reg0, temp_reg0
	brne heap_write_exception_xptr
#endif	
memmap_perms_check_exit_xptr:
	pop temp_reg0		       ; STACK: (temp_reg0)|zptr_h|zptr_l|SREG|temp_reg1
	st  X, temp_reg0
	pop zptr_h                     ; STACK: (zptr_h)|zptr_l|SREG|temp_reg1
	pop zptr_l		       ; STACK: (zptr_l)|SREG|temp_reg1
	pop temp_reg1		       ; STACK: (SREG)|temp_reg1
	out __SREG__, temp_reg1	       ; Restore SREG	
	pop temp_reg1                  ; STACK: (temp_reg1)
	ret
stack_bound_checker_xptr:
	lds zptr_l, domain_stack_bound 		;  
	lds zptr_h, (domain_stack_bound + 1)	; Z = domain_stack_bound
	cp xptr_l, zptr_l		; 	
	cpc xptr_h, zptr_h		;
	brcs memmap_perms_check_exit_xptr	; if (X < domain_stack_bound) memmap_perms_check_exit_xptr;
stack_write_exception_xptr:	
	eor param1, param1
	ldi param0, MEMMAP_STACK_WRITE_EXCEPTION
	call sfi_exception	
heap_write_exception_xptr:	
	eor param1, param1
	ldi param0, MEMMAP_HEAP_WRITE_EXCEPTION
	call sfi_exception  

.global ker_memmap_perms_check_yptr
ker_memmap_perms_check_yptr:
	push temp_reg1          	; STACK: temp_reg1
	in temp_reg1, __SREG__  	; temp_reg1 = SREG
	cli				; Disable interrupts
	push temp_reg1                  ; STACK: SREG|temp_reg1
	push zptr_l			; STACK: zptr_l|SREG|temp_reg1
	push zptr_h			; STACK: zptr_h|zptr_l|SREG|temp_reg1
	push temp_reg0                  ; STACK: temp_reg0|zptr_h|zptr_l|SREG|temp_reg1
	in zptr_l, __SP_L__
	in zptr_h, __SP_H__
	cp yptr_l, zptr_l		; 
	cpc yptr_h, zptr_h		; 
	brcc stack_bound_checker_yptr	; if (Y >= Stack Pointer) then do a stack bound check
	movw zptr_l, yptr_l     	; zptr_h:zptr_l = yptr_h:yptr_l
	lsr zptr_h			; if (NUM_DOMAINS == 8) zptr_h:zptr_l >>= 4 ;
	ror zptr_l		 	; if (NUM_DOMAINS == 2) zptr_h:zptr_l >>= 5;
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
#ifdef SFI_DOMS_2
	lsr zptr_h
	ror zptr_l			; zptr_h:zptr_l = memmap_offset
#endif	
	subi zptr_l, lo8(-(memmap))
	sbci zptr_h, hi8(-(memmap))     ; zptr_h:zptr_l = &memmap[memmap_offset]
	ld  temp_reg1,Z                 ; temp_reg1 = perms_byte = memmap[memmap_offset]
	mov zptr_l, yptr_l              ; zptr_l = yptr_l
	eor zptr_h, zptr_h		; clear zptr_h
#ifdef SFI_DOMS_2
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(MEMMAP_PERMS_BM_LUT))
	sbci zptr_h,hi8(-(MEMMAP_PERMS_BM_LUT))	; zptr_h:zptr_l = &MEMMAP_PERMS_BM_LUT[address_offset]
	lpm temp_reg0,Z                 ; temp_reg0 = perms_mask = MEMMAP_PERMS_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = perms_byte & perms_mask
	and temp_reg0, temp_reg0
	brne heap_write_exception_yptr  ; if (temp_reg0 != 0) then goto heap_write_exception_yptr
	eor zptr_h, zptr_h		; clear zptr_h
	mov zptr_l, yptr_l              ; zptr_l = yptr_l
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0,Z			; temp_reg0 = blkdhr_mask = BLK_HDR_CHECK_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = (perms_byte & blkhdr_mask)
	and temp_reg0, temp_reg0
	brne heap_write_exception_yptr  ; if (temp_reg0 != 0) then goto heap_write_exception_yptr
#endif	
#ifdef SFI_DOMS_8
	andi zptr_l, 0x08               ; zptr_l indicates which nibble we are interested in
	cpse zptr_h, zptr_l		; if (zptr_h == zptr_l) i.e. if (0 == zptr_l), skip next instr.
	swap temp_reg1			; Swap nibbles of perms
	mov zptr_l, temp_reg1
	andi zptr_l, 0x07
	lds zptr_h, curr_dom_id
	cp zptr_h, zptr_l
	brne heap_write_exception_yptr
	mov zptr_l, yptr_l
	andi zptr_l, 0x07
	eor zptr_h, zptr_h
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0, Z
	and temp_reg0, temp_reg1
	and temp_reg0, temp_reg0
	brne heap_write_exception_yptr
#endif	
memmap_perms_check_exit_yptr:
	pop temp_reg0		       ; STACK: (temp_reg0)|zptr_h|zptr_l|SREG|temp_reg1
	st  Y, temp_reg0
	pop zptr_h                     ; STACK: (zptr_h)|zptr_l|SREG|temp_reg1
	pop zptr_l		       ; STACK: (zptr_l)|SREG|temp_reg1
	pop temp_reg1		       ; STACK: (SREG)|temp_reg1
	out __SREG__, temp_reg1	       ; Restore SREG	
	pop temp_reg1                  ; STACK: (temp_reg1)
	ret
stack_bound_checker_yptr:
	lds zptr_l, domain_stack_bound 		;  
	lds zptr_h, (domain_stack_bound + 1)	; Z = domain_stack_bound
	cp yptr_l, zptr_l		; 	
	cpc yptr_h, zptr_h		;
	brcs memmap_perms_check_exit_yptr	; if (Y < domain_stack_bound) memmap_perms_check_exit_yptr;
stack_write_exception_yptr:	
	eor param1, param1
	ldi param0, MEMMAP_STACK_WRITE_EXCEPTION
	call sfi_exception	
heap_write_exception_yptr:	
	eor param1, param1
	ldi param0, MEMMAP_HEAP_WRITE_EXCEPTION
	call sfi_exception  
	
	
	
.global ker_memmap_perms_check_zptr
ker_memmap_perms_check_zptr:
	push temp_reg1          	; STACK: temp_reg1
	in temp_reg1, __SREG__  	; temp_reg1 = SREG
	cli				; Disable interrupts
	push temp_reg1                  ; STACK: SREG|temp_reg1
	push xptr_l			; STACK: xptr_l|SREG|temp_reg1
	push xptr_h			; STACK: xptr_h|xptr_l|SREG|temp_reg1
	push temp_reg0                  ; STACK: temp_reg0|xptr_h|xptr_l|SREG|temp_reg1
	movw xptr_l, zptr_l
	in zptr_l, __SP_L__
	in zptr_h, __SP_H__
	cp xptr_l, zptr_l		; 
	cpc xptr_h, zptr_h		; 
	brcc stack_bound_checker_zptr	; if (X >= Stack Pointer) then do a stack bound check
	movw zptr_l, xptr_l     	; zptr_h:zptr_l = xptr_h:xptr_l
	lsr zptr_h			; if (NUM_DOMAINS == 8) zptr_h:zptr_l >>= 4 ;
	ror zptr_l		 	; if (NUM_DOMAINS == 2) zptr_h:zptr_l >>= 5;
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
	lsr zptr_h
	ror zptr_l
#ifdef SFI_DOMS_2
	lsr zptr_h
	ror zptr_l			; zptr_h:zptr_l = memmap_offset
#endif	
	subi zptr_l, lo8(-(memmap))
	sbci zptr_h, hi8(-(memmap))     ; zptr_h:zptr_l = &memmap[memmap_offset]
	ld  temp_reg1,Z                 ; temp_reg1 = perms_byte = memmap[memmap_offset]
	mov zptr_l, xptr_l              ; zptr_l = xptr_l
	eor zptr_h, zptr_h		; clear zptr_h
#ifdef SFI_DOMS_2
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(MEMMAP_PERMS_BM_LUT))
	sbci zptr_h,hi8(-(MEMMAP_PERMS_BM_LUT))	; zptr_h:zptr_l = &MEMMAP_PERMS_BM_LUT[address_offset]
	lpm temp_reg0,Z                 ; temp_reg0 = perms_mask = MEMMAP_PERMS_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = perms_byte & perms_mask
	and temp_reg0, temp_reg0
	brne heap_write_exception_zptr  ; if (temp_reg0 != 0) then goto heap_write_exception_zptr
	eor zptr_h, zptr_h		; clear zptr_h
	mov zptr_l, xptr_l              ; zptr_l = xptr_l
	andi zptr_l, 0x1F               ; zptr_l = Address Offset
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0,Z			; temp_reg0 = blkdhr_mask = BLK_HDR_CHECK_BM_LUT[address_offset]
	and temp_reg0, temp_reg1        ; temp_reg0 = (perms_byte & blkhdr_mask)
	and temp_reg0, temp_reg0
	brne heap_write_exception_zptr  ; if (temp_reg0 != 0) then goto heap_write_exception_zptr
#endif	
#ifdef SFI_DOMS_8
	andi zptr_l, 0x08               ; zptr_l indicates which nibble we are interested in
	cpse zptr_h, zptr_l		; if (zptr_h == zptr_l) i.e. if (0 == zptr_l), skip next instr.
	swap temp_reg1			; Swap nibbles of perms
	mov zptr_l, temp_reg1
	andi zptr_l, 0x07
	lds zptr_h, curr_dom_id
	cp zptr_h, zptr_l
	brne heap_write_exception_zptr
	mov zptr_l, xptr_l
	andi zptr_l, 0x07
	eor zptr_h, zptr_h
	subi zptr_l,lo8(-(BLK_HDR_CHECK_BM_LUT))
	sbci zptr_h,hi8(-(BLK_HDR_CHECK_BM_LUT)) ; zptr_h:zptr_l = &BLK_HDR_CHECK_BM_LUT[address_offset]
	lpm temp_reg0, Z
	and temp_reg0, temp_reg1
	and temp_reg0, temp_reg0
	brne heap_write_exception_zptr
#endif	
memmap_perms_check_exit_zptr:
	pop temp_reg0		       ; STACK: (temp_reg0)|xptr_h|xptr_l|SREG|temp_reg1
	st  X, temp_reg0
	movw zptr_l, xptr_l
	pop xptr_h                     ; STACK: (xptr_h)|zptr_l|SREG|temp_reg1
	pop xptr_l		       ; STACK: (xptr_l)|SREG|temp_reg1
	pop temp_reg1		       ; STACK: (SREG)|temp_reg1
	out __SREG__, temp_reg1	       ; Restore SREG	
	pop temp_reg1                  ; STACK: (temp_reg1)
	ret
stack_bound_checker_zptr:
	lds zptr_l, domain_stack_bound 		;  
	lds zptr_h, (domain_stack_bound + 1)	; Z = domain_stack_bound
	cp xptr_l, zptr_l		; 	
	cpc xptr_h, zptr_h		;
	brcs memmap_perms_check_exit_zptr	; if (X < domain_stack_bound) memmap_perms_check_exit_zptr;
stack_write_exception_zptr:	
	eor param1, param1
	ldi param0, MEMMAP_STACK_WRITE_EXCEPTION
	call sfi_exception	
heap_write_exception_zptr:	
	eor param1, param1
	ldi param0, MEMMAP_HEAP_WRITE_EXCEPTION
	call sfi_exception  
