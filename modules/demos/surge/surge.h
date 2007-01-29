#ifndef _SURGE_H_
#define _SURGE_H_



//! This can be made a compile time configuration
#define SURGE_BASE_STATION_ADDRESS 1

//! Surge Timer Settings
enum
  {
    INITIAL_TIMER_RATE = 8 * 1024,
    TIMER_GETADC_COUNT = 1
  };


//-------------------------------------------------------
// MODULE MESSAGE TYPES
//-------------------------------------------------------
enum
  {
    MSG_SURGE_PKT = (MOD_MSG_START + 0)
  };

//! Surge Packet Types - From the TOS Implementation
enum {
  SURGE_TYPE_SENSORREADING = 0,
  SURGE_TYPE_ROOTBEACON = 1,
  SURGE_TYPE_SETRATE = 2,
  SURGE_TYPE_SLEEP = 3,
  SURGE_TYPE_WAKEUP = 4,
  SURGE_TYPE_FOCUS = 5,
  SURGE_TYPE_UNFOCUS = 6
}; 


//! Surge Message Structure - Modified from the TOS Implementation
typedef struct SurgeMsg {
  uint8_t type;
  uint16_t reading;
  uint16_t originaddr;
  //  uint16_t parentaddr;
  uint32_t seq_no;
  //  uint8_t light;
  //  uint8_t temp;
  //  uint8_t magx;
  //  uint8_t magy;
  //  uint8_t accelx;
  //  uint8_t accely;
} __attribute__ ((packed)) SurgeMsg;



#ifndef _MODULE_
extern mod_header_ptr surge_get_header();
#endif //_MODULE_

#endif
