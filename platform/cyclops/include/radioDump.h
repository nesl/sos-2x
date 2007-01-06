#ifndef RADIODUMP_H
#define RADIODUMP_H


#include <message_types.h>

#define RADIO_PAYLOAD_LEN 64



 typedef struct radioDumpFrame_s
{
    uint16_t seq;                                      //Remaining number of bytes sequence,	// Kevin - reduce this down to 8 bit from 16 bit
    uint8_t payload[RADIO_PAYLOAD_LEN]; //Actual payload in the frame	
}__attribute__((packed)) radioDumpFrame_t;


 typedef struct framCount_s
{
    uint32_t frameSequence[2]; // 64 bit for each frame received	
}__attribute__((packed)) framCount_t;


//extern uint8_t memoryDump(uint8_t *start, uint16_t length);

extern int8_t radioDumpControl_init();
#endif

