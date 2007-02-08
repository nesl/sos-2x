
.section .sos_sysjmptbl,"ax"
.global  sos_sysjmptbl
sos_sysjmptbl:
br #ker_sys_fnptr_call
br #ker_sys_malloc
br #ker_sys_realloc 
br #ker_sys_free 
br #ker_sys_msg_take_data 
br #ker_sys_timer_start 
br #ker_sys_timer_restart 
br #ker_sys_timer_stop 
br #ker_sys_post 
br #ker_sys_post_link 
br #ker_sys_post_value 
br #ker_hw_type 
br #ker_id 
br #ker_rand 
br #ker_systime32 
br #ker_sys_sensor_get_data 
br #ker_led 
br #ker_sys_get_module_state 
br #ker_sys_fntable_subscribe    ; 18
br #ker_sys_change_own    ; 19
br 0    ; 20 // Ram - Do not use .. being used for SFI in AVR
br #ker_sys_codemem_read 	; 21
br 0    ; 22
br 0 
br 0    ; 24
br 0 
br 0    ; 26
br 0 
br 0    ; 28
br 0 
br 0    ; 30
br 0 
