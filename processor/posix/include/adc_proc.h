#ifndef _ADC_PROC_H
#define _ADC_PROC_H



enum {
  TOSH_ADC_PORTMAPSIZE = 12
};

enum 
{
  TOSH_ACTUAL_CC_RSSI_PORT = 0,
  TOSH_ACTUAL_VOLTAGE_PORT = 7,
  TOSH_ACTUAL_BANDGAP_PORT = 30,  // 1.23 Fixed bandgap reference
  TOSH_ACTUAL_GND_PORT     = 31   // GND 
};

enum 
{
  TOS_ADC_CC_RSSI_PORT = 0,
  TOS_ADC_VOLTAGE_PORT = 7,
  TOS_ADC_BANDGAP_PORT = 10,
  TOS_ADC_GND_PORT     = 11
};

/**
 * @brief ADC functions
 */

/** 
 * @brief init function for ADC
 */
void ADCControl_init();


/**
 * @brief bind port with callback 
 * The port will be used in ADC_getData and ADC_getContinuousData
 * Note that the callback will be executed in interrupt handler
 * This means that system developer will have to take care 
 * racing conditions
 */

extern int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcPort, uint8_t driverpid);

/**
 * @brief get data from a port 
 */
extern int8_t ker_adc_proc_getData(uint8_t port);



#endif // _ADC_PROC_H 

