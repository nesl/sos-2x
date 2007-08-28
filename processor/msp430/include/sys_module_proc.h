#ifndef _SYS_MODULE_PROC_H_
#define _SYS_MODULE_PROC_H_

// User ISR Registration Controller
#include <interrupt_ctrl.h>

#include <adc_driver.h>

/**
 * \ingroup system_api
 * \defgroup adc ADC driver API
 *
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (*ker_sys_adc_bind_channel_func_t)(uint16_t channels, uint8_t control_cb_fid,
				uint8_t cb_fid, void *config);
/// \endcond

static inline int8_t sys_adc_bind_channel (uint16_t channels, uint8_t control_cb_fid,
				uint8_t cb_fid, void *config)
{
		return ((ker_sys_adc_bind_channel_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*48))
				(channels, control_cb_fid, cb_fid, config);
}


/**
 * TODO: Comment this chunk of code.
 */
/// \cond NOTYPEDEF
typedef int8_t (*ker_sys_adc_unbind_channel_func_t) (uint16_t channels);
/// \endcond

static inline int8_t sys_adc_unbind_channel (uint16_t channels)
{
		return ((ker_sys_adc_unbind_channel_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*49))
				(channels);
}


/**
 * TODO: Comment this chunk of code.
 */
/// \cond NOTYPEDEF
typedef int8_t (*ker_adc_get_data_func_t)(uint8_t command, sos_pid_t app_id, uint16_t channels,
				sample_context_t *param, void *context);
/// \endcond

static inline int8_t sys_adc_get_data(uint8_t command, sos_pid_t app_id, uint16_t channels,
				sample_context_t *param, void *context)
{
		return ((ker_adc_get_data_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*50))
				(command, app_id, channels, param, context);
}


/**
 * TODO: Comment this chunk of code.
 */
/// \cond NOTYPEDEF
typedef int8_t (*ker_adc_stop_data_func_t) (sos_pid_t app_id, uint16_t channels);
/// \endcond

static inline int8_t sys_adc_stop_data (sos_pid_t app_id, uint16_t channels)
{
		return ((ker_adc_stop_data_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*51)) (app_id, channels);
}

/* @} */
/**
 * \ingroup system_api
 * \defgroup interrupt User ISR Registration Service
 *
 * @{
 */
/// \cond NOTYPEDEF
typedef int8_t (*ker_sys_register_isr_func_t)(sos_interrupt_t int_id, uint8_t isr_fid);
/// \endcond

static inline int8_t sys_register_isr(sos_interrupt_t int_id, uint8_t isr_fid) {
		return ((ker_sys_register_isr_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*52))
				(int_id, isr_fid);

}

/// \cond NOTYPEDEF
typedef int8_t (*ker_sys_deregister_isr_func_t)(sos_interrupt_t int_id);
/// \endcond

static inline int8_t sys_deregister_isr(sos_interrupt_t int_id) {
		return ((ker_sys_deregister_isr_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*53))
				(int_id);

}

/* @} */

#endif

