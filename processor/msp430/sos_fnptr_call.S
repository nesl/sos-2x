; r12 stores previous interrupt flag
; r13 stores the PID stack pointer
; 6(r15) has the destination PID
; &r15 has the address to destination function
; &pid_sp has the address to the top of PID stack

.section .text
.global ker_sys_fnptr_call
ker_sys_fnptr_call:
	push r12            ;
	push r13            ;
	mov r2, r12         ;
	and.b #8, r12       ;
	dint
		mov &pid_sp, r13          ; get stack pointer for pid
		mov.b &curr_pid, 0(r13)   ;
		inc &pid_sp               ; increment pid_sp
		cmp.b #255, 6(r15)       ;
		jeq label_store_curr_pid  ;
		mov.b 6(r15), &curr_pid;
label_store_curr_pid:
	
	cmp.b #0, r12       ;
	jz label_call_dy_func              ;
	eint
label_call_dy_func:
	pop  r13            ;
	pop  r12            ;
	call  @r15           ; branch to real implementation
	
	push r12            ;
	push r13            ;
	mov r2, r12         ;
	and.b #8, r12       ;
	dint
		dec &pid_sp               ;
		mov &pid_sp, r13          ; get stack pointer for pid
    	mov.b 0(r13), &curr_pid   ; unset the PID
	cmp.b #0, r12       ;
	jz label_return_dy_func              ;
	eint
label_return_dy_func:
	pop  r13            ;
	pop  r12            ;
	ret                 ;

;;; End of file
