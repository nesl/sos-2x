/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <sys_module.h>

#include <sensor.h>
#include <adc_proc_common.h>

#include <sensordrivers/mica2/include/battery.h>

typedef struct battery_state {
    uint8_t battery_voltage;
} battery_sensor_state_t;


// function registered with kernel sensor component
static int8_t battery_control(func_cb_ptr cb, uint8_t cmd, void *data);

// data ready callback registered with adc driver
int8_t battery_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);

static int8_t battery_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
mod_id : BATTERY_SENSOR_PID,
         state_size : sizeof(battery_sensor_state_t),
         num_timers : 0,
         num_sub_func : 0,
         num_prov_func : 2,
         platform_type : HW_TYPE,
         processor_type : MCU_TYPE,
         code_id : ehtons(BATTERY_SENSOR_PID),
         module_handler : battery_msg_handler,
         funct : {
             {battery_control, "cCw2", BATTERY_SENSOR_PID, SENSOR_CONTROL_FID},
             {battery_data_ready_cb, "cCS3", BATTERY_SENSOR_PID, SENSOR_DATA_READY_FID},
         },
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 */
int8_t battery_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags) {
    sys_sensor_data_ready(MICA2_BATTERY_SID, value, flags);
    return SOS_OK;
}


static inline void battery_on() {
    SET_BAT_MON();
    SET_BAT_MON_DD_OUT();
}
static inline void battery_off() {
    SET_BAT_MON_DD_IN();
    CLR_BAT_MON();
}

static int8_t battery_control(func_cb_ptr cb, uint8_t cmd, void* data) {\

    switch (cmd) {
        case SENSOR_GET_DATA_CMD:
            return sys_adc_proc_getData(MICA2_BATTERY_SID, 0);

        case SENSOR_ENABLE_CMD:
            battery_on();
            break;

        case SENSOR_DISABLE_CMD:
            battery_off();
            break;

        case SENSOR_CONFIG_CMD:
            // no configuation
            if (data != NULL) {
                sys_free(data);
            }
            break;

        default:
            return -EINVAL;
    }
    return SOS_OK;
}


int8_t battery_msg_handler(void *state, Message *msg)
{

    switch (msg->type) {

        case MSG_INIT:
            // bind adc channel and register callback pointer
            sys_adc_proc_bindPort(MICA2_BATTERY_SID, 
                    MICA2_BATTERY_CH, 
                    BATTERY_SENSOR_PID,  
                    SENSOR_DATA_READY_FID);

            // register with kernel sensor interface
            sys_sensor_register(BATTERY_SENSOR_PID, 
                    MICA2_BATTERY_SID, 
                    SENSOR_CONTROL_FID, 
                    NULL);
            break;

        case MSG_FINAL:
            // shutdown sensor
            battery_off();
            //  unregister ADC port
            sys_adc_proc_unbindPort(BATTERY_SENSOR_PID, MICA2_BATTERY_SID);
            // unregister sensor
            sys_sensor_deregister(BATTERY_SENSOR_PID, MICA2_BATTERY_SID);
            break;

        default:
            return -EINVAL;
            break;
    }
    return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr battery_get_header() {
    return sos_get_header_address(mod_header);
}
#endif

