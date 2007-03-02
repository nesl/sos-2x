#include <sos.h>
#include <module.h>
#include <sys_module.h>
#include <signal.h>
#include <msp430/adc12.h>
#include <msp430/dma.h>
#include "adc.h"

#define LED_DEBUG
#include <led_dbg.h>

typedef struct adc_state {
    uint8_t spid;
} adc_state_t;

static int8_t adc_msg_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = ADC_PID,
    .state_size     = sizeof(adc_state_t),
    .num_timers     = 0,
    .num_sub_func   = 0,
    .num_prov_func  = 0,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(ADC_PID),
    .module_handler = adc_msg_handler,
};

static int8_t adc_msg_handler(void *state, Message *msg){
    
    switch(msg->type) {
        case MSG_INIT:
            P6DIR &=~(0x01); //input
            P6SEL |= 0x01; //ADC12 function
            break;  
    
        case MSG_FINAL:
            adc_stop();  
            break;  

        default:
            return -EINVAL;
    }
    return SOS_OK;
}

interrupt (DACDMA_VECTOR) dac_dma_interrupt (){
    adc_state_t* s = (adc_state_t*)ker_get_module_state(ADC_PID);

    if((DMA0CTL & DMAIFG) == DMAIFG){
        adc_stop( );
        DMA0CTL &= ~DMAIFG;

        // Access your data here
        LED_DBG(LED_YELLOW_TOGGLE);
        sys_post_value(s->spid, ADC_DATA_READY_MSG, 0, SOS_MSG_RELEASE);
    }
}

int8_t adc_start(uint8_t spid, uint16_t* start, uint16_t length, enum adcRate rate){
  adc_state_t* s = (adc_state_t*)ker_get_module_state(ADC_PID);
  s->spid = spid;
    
  if((DMA0CTL & DMAEN) == DMAEN){ //a DMA trasnfer is active
   return 1; 
  }  
  //stop new ADC transfers (stop timerB)
  TBCTL =0;
  DMA0CTL =0; //stop DMA transfers
  ADC12CTL0 &= ~(ENC); //disable converter
  //wait for pending conversions to finish
  while((ADC12CTL1 & 0x0001) != 0){}; 
  
  ADC12CTL0 = (ADC12ON+SHT0_0 +SHT1_0);    // Turn on ADC12
  ADC12CTL1 = (SHS_3 + ADC12SSEL_2 + CONSEQ_2);           
  ADC12MCTL0 = (SREF_0 |INCH_0);
  ADC12IFG =0; //clear any pending interrupts
  
  //configure TimerB
  TBCCR0 = rate;
  TBCCR1 = rate/2;
  TBCCTL1 = (OUTMOD_3);
  TBCTL = (TBSSEL_2);//clock source SMCLK
  
  //configure and start DMA
  //configure DMA channel
  DMACTL0 =  DMA0TSEL_6;
  DMACTL1 =0;  
  //set DMA addresses
  DMA0DA = (unsigned int) start;
  DMA0SZ = length;
  DMA0SA = (unsigned int) &(ADC12MEM0);
  //set channel 0 properties
  DMA0CTL = ( DMADT_0 | DMAIE | DMASWDW |DMASRCINCR_0  |DMADSTINCR_3 |DMAEN); 
  
  //start TimerB (and the conversions)
  ADC12CTL0 |= ENC;  
  TBCTL |= MC_1; //counts up to TBCL0
  return 0;
}


void adc_stop( ){
  //stop pending DMAs 
  
  //stop ADC trigger (stop timerB)
  TBCTL =0;
  //stop ADC
  //while((ADC12CTL1 & 0x0001) != 0){}; //wait for pending conversions to complete
  ADC12CTL0 &= ~(ENC); //disable converter
  //turn the ADC off 
  ADC12CTL0 &= ~ADC12ON;  
}


#ifndef _MODULE_
mod_header_ptr adc_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif

