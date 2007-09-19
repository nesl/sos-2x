
.section .sos_sysjmptbl,"ax"
.global  sos_sysjmptbl
sos_sysjmptbl:
br #ker_sys_fnptr_call      ; 0
br #ker_sys_malloc          ; 1
br #ker_sys_realloc         ; 2
br #ker_sys_free            ; 3
br #ker_sys_msg_take_data   ; 4
br #ker_sys_timer_start     ; 5
br #ker_sys_timer_restart   ; 6
br #ker_sys_timer_stop      ; 7
br #ker_sys_post            ; 8
br #ker_sys_post_link       ; 9
br #ker_sys_post_value      ; 10
br #ker_hw_type             ; 11
br #ker_id                  ; 12
br #ker_rand                ; 13
br #ker_systime32           ; 14
br 0	; br #ker_sensor_get_data     ; 15 // Old sensing API
br #ker_led                 ; 16
br #ker_sys_get_module_state    ; 17
br #ker_sys_fntable_subscribe   ; 18
br #ker_sys_change_own      ; 19
br 0    ; 20 // Ram - Do not use .. being used for SFI in AVR
br #ker_sys_codemem_read 	; 21
br #ker_sys_shm_open        ; 22
br #ker_sys_shm_update      ; 23
br #ker_sys_shm_close       ; 24
br #ker_sys_shm_get         ; 25
br #ker_sys_shm_wait        ; 26
br #ker_sys_shm_stopwait    ; 27
br #ker_get_current_pid     ; 28
br #ker_get_caller_pid      ; 29
br #ker_sys_routing_register    ; 30
br 0	; br #ker_adc_proc_bindPort   ; 31 // Old sensing API
br 0	; br #ker_adc_proc_unbindPort ; 32 // Old sensing API
br 0	; br #ker_adc_proc_getData    ; 33 // Old sensing API
br 0	; br #ker_sensor_register     ; 34 // Old sensing API
br 0	; br #ker_sensor_deregister   ; 35 // Old sensing API
br 0	; br #ker_sensor_data_ready   ; 36 // Old sensing API
br 0	; br #ker_sensor_enable       ; 37 // Old sensing API
br 0	; br #ker_sensor_disable      ; 38 // Old sensing API
br 0    ; br #ker_i2c_reserve_bus     ; 39
br 0    ; br #ker_i2c_release_bus     ; 40
br 0    ; br #ker_i2c_send_data       ; 41
br 0    ; br #ker_i2c_read_data       ; 42
br #post_longer             			; 43
br #ker_sys_sensor_start_sampling		; 44
br #ker_sys_sensor_stop_sampling		; 45
br # ker_sys_sensor_driver_register		; 46
br #ker_sys_sensor_driver_deregister	; 47
br #ker_sys_adc_bind_channel			; 48
br #ker_sys_adc_unbind_channel			; 49
br #ker_adc_get_data					; 50
br #ker_adc_stop_data					; 51
br #ker_sys_register_isr				; 52    // Used for user interrupt controller in msp430
br #ker_sys_deregister_isr				; 53    // Used for user interrupt controller in msp430
br 0	; br #ker_sensor_control		; 54 // Old sensing API
br 0									; 55
