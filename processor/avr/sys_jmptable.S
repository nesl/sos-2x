
.section .sos_sysjmptbl,"ax"
.global  sos_sysjmptbl
sos_sysjmptbl:
jmp ker_sys_fnptr_call     ; 0
jmp ker_sys_malloc         ; 1
jmp ker_sys_realloc        ; 2
jmp ker_sys_free           ; 3
jmp ker_sys_msg_take_data  ; 4
jmp ker_sys_timer_start    ; 5
jmp ker_sys_timer_restart  ; 6
jmp ker_sys_timer_stop     ; 7
jmp ker_sys_post           ; 8
jmp ker_sys_post_link      ; 9
jmp ker_sys_post_value     ; 10
jmp ker_hw_type            ; 11
jmp ker_id                 ; 12
jmp ker_rand               ; 13
jmp ker_systime32          ; 14
jmp ker_sys_sensor_get_data
jmp ker_led                ; 16
jmp ker_sys_get_module_state ; 17
jmp ker_sys_fntable_subscribe ; 18
jmp ker_sys_change_own     ; 19
#ifdef SOS_SFI	
jmp ker_sys_fnptr_from_ker_call ; 20
#else
jmp 0                      ; 20
#endif	
jmp ker_sys_codemem_read   ; 21
jmp 0                      ; 22
jmp 0                      ; 23
jmp 0                      ; 24
jmp 0                      ; 25
jmp 0                      ; 26
jmp 0                      ; 27
jmp 0                      ; 28
jmp 0                      ; 29
jmp 0                      ; 30
jmp 0                      ; 31
