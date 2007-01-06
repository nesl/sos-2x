#ifndef SERIALDUMP_H
#define SERIALDUMP_H


#include <message_types.h>

#define UART_PAYLOAD_LEN 64



typedef struct serialDumpFrame_s {
  uint16_t seq;                      //Remaining number of bytes sequence,
  uint8_t payload[UART_PAYLOAD_LEN]; //Actual payload in the frame	
}__attribute__((packed)) serialDumpFrame_t;


//extern uint8_t memoryDump(uint8_t *start, uint16_t length);

extern int8_t serialDumpControl_init();
#endif

