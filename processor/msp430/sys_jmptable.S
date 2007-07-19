
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
br #ker_sensor_get_data 
br #ker_led 
br #ker_sys_get_module_state 
br #ker_sys_fntable_subscribe    ; 18
br #ker_sys_change_own    ; 19
br 0    ; 20 // Ram - Do not use .. being used for SFI in AVR
br #ker_sys_codemem_read 	; 21
br #ker_sys_shm_open    ; 22
br #ker_sys_shm_update 
br #ker_sys_shm_close    ; 24
br #ker_sys_shm_get 
br #ker_sys_shm_wait    ; 26
br #ker_sys_shm_stopwait 
br #ker_get_current_pid    ; 28
br #ker_get_caller_pid 
br #ker_sys_routing_register    ; 30
br #ker_adc_proc_bindPort 
br #ker_adc_proc_unbindPort		; 32
br #ker_adc_proc_getData
br #ker_sensor_register         ; 34
br #ker_sensor_deregister
br #ker_sensor_data_ready       ; 36
br #ker_sensor_enable
br #ker_sensor_disable     ;38
;br 0
;br #ker_adc_proc_getPeriodicData	; 34
;br #ker_adc_proc_stopPeriodicData
