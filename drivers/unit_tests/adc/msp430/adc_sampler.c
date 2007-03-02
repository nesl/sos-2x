#include <sos.h>
#include <module.h>
#include <sys_module.h>
#include <adc/msp430/adc.h>

#define LED_DEBUG
#include <led_dbg.h>

typedef struct adc_sampler_state {
    uint8_t spid;
    uint16_t data[1020];
} adc_sampler_state_t;

static int8_t adc_sampler_msg_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = DFLT_APP_ID0,
    .state_size     = sizeof(adc_sampler_state_t),
    .num_timers     = 0,
    .num_sub_func   = 0,
    .num_prov_func  = 0,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(DFLT_APP_ID0),
    .module_handler = adc_sampler_msg_handler,
};

static int8_t adc_sampler_msg_handler(void *state, Message *msg){
    adc_sampler_state_t* s = (adc_sampler_state_t*)state;
    int i;
    
    switch(msg->type) {
        case MSG_INIT:
            s->spid = msg->did;
            adc_start(s->spid, s->data, 1020, EIGHT_KHZ);
            LED_DBG(LED_RED_ON);
            break;
        case ADC_DATA_READY_MSG:
            LED_DBG(LED_GREEN_TOGGLE);
            //adc_start(s->spid, s->data, 100, EIGHT_KHZ);
            
            for(i=0; i<1020; i+= 60){
                sys_post_uart(s->spid, 89, 60, &(s->data[i]), 0, UART_ADDRESS);
            }

            break;
        case MSG_FINAL:
            break;
        default:
            return -EINVAL;
    }
    return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr adc_sampler_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif

