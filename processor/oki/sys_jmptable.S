
.section .sos_sysjmptbl,"ax"
.global  sos_sysjmptbl
sos_sysjmptbl:
bl ker_sys_fnptr_call
bl ker_sys_malloc
bl ker_sys_realloc
bl ker_sys_free
bl ker_sys_msg_take_data
bl ker_sys_timer_start
bl ker_sys_timer_restart
bl ker_sys_timer_stop
bl ker_sys_post
bl ker_sys_post_link
bl ker_sys_post_value
bl ker_hw_type
bl ker_id
bl ker_rand
bl ker_systime32
bl ker_sys_sensor_get_data
