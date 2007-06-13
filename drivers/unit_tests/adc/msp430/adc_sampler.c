#include <sos.h>
#include <module.h>
#include <sys_module.h>
#include <adc/msp430/adc.h>
#include <bitsop.h>

#define LED_DEBUG
#include <led_dbg.h>

#define ADC_TID 0

typedef struct adc_sampler_state {
    uint8_t spid;
    uint16_t data[1020];
    uint16_t index;
} adc_sampler_state_t;

static int8_t adc_sampler_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = DFLT_APP_ID0,
    .state_size     = sizeof(adc_sampler_state_t),
    .num_sub_func   = 0,
    .num_prov_func  = 0,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(DFLT_APP_ID0),
    .module_handler = adc_sampler_msg_handler,
};

static int8_t adc_sampler_msg_handler(void *state, Message *msg){
    adc_sampler_state_t* s = (adc_sampler_state_t*)state;
    
    switch(msg->type) {
        case MSG_INIT:
            s->spid = msg->did;
            SETBITHIGH(P5OUT, 0);
            sys_timer_start(ADC_TID, 3048, TIMER_REPEAT);
            break;
        case ADC_DATA_READY_MSG:
            LED_DBG(LED_GREEN_ON);
            //adc_start(s->spid, s->data, 100, EIGHT_KHZ);
            s->index = 0; 
            sys_post_net(s->spid, 89, 60, &(s->data[s->index]), SOS_MSG_RELIABLE, BCAST_ADDRESS);
            s->index += 60;
            break;

        case MSG_PKT_SENDDONE:
            if (s->index<1020){
                sys_post_net(s->spid, 89, 60, &(s->data[s->index]), SOS_MSG_RELIABLE, BCAST_ADDRESS);
                s->index += 60;
            } else {
                LED_DBG(LED_GREEN_OFF);
                //SETBITHIGH(P5OUT, 0);
            }
            break;
        case MSG_TIMER_TIMEOUT:
            adc_start(s->spid, s->data, 1020, EIGHT_KHZ);
            
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

