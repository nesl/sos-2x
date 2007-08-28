; r11 stores the PC while transferring it to pid stack
; r12 stores previous interrupt flag
; r13 stores the PID stack pointer
; 6(r15) has the destination PID
; &r15 has the address to destination function
; &pid_sp has the address to the top of PID stack

.section .text
.global ker_sys_fnptr_call
ker_sys_fnptr_call:
	push r11			;
	mov 2(r1), r11		; save PC into r11 from stack
	push r12            ;
	push r13            ;
	mov r2, r12         ;
	and.b #8, r12       ;
	dint
		mov &pid_sp, r13        ; get stack pointer for pid stack
		mov r11, 0(r13)			; push PC into pid stack
		incd r13				; 
		mov.b &curr_pid, 0(r13) ;
		incd r13				; increment twice to keep pid stack word aligned
		mov r13, &pid_sp		; increment pid_sp by 3
		cmp.b #255, 6(r15)      ;
		jeq label_store_curr_pid;
		mov.b 6(r15), &curr_pid	;
label_store_curr_pid:
	
	cmp.b #0, r12       ;
	jz label_call_dy_func              ;
	eint
label_call_dy_func:
	pop  r13            ;
	pop  r12            ;
	pop  r11			;
	add #2, r1			; additional pop to remove saved PC
	call  @r15          ; branch to real implementation

	push #0				; make place for saved PC
	push r11			;
	push r12            ;
	push r13            ;
	mov r2, r12         ;
	and.b #8, r12       ;
	dint
		decd &pid_sp              	;
		mov &pid_sp, r13          	; get stack pointer for pid
    	mov.b 0(r13), &curr_pid   	; unset the PID
		decd r13					;
		mov 0(r13), r11				; get saved PC from pid stack
		mov r13, &pid_sp			;
	cmp.b #0, r12       ;
	jz label_return_dy_func              ;
	eint
label_return_dy_func:
	pop  r13            ;
	pop  r12            ;
	mov r11, 2(r1)		; put saved PC in its place
	pop r11				;
	ret                 ;

;;; End of file
