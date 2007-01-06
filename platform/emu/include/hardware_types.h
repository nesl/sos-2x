
#ifndef _SOS_HARDWARD_TYPES_H_
#define _SOS_HARDWARD_TYPES_H_
#include <hardware_proc.h>
#include <sos_info.h>
#include <stdio.h>

void process_user_input(char *user_input);

#define DEBUG_BUFFER_SIZE 1024
#define DEBUG(arg...){                                     \
    char dbuffer[DEBUG_BUFFER_SIZE];                       \
    int16_t dsize = DEBUG_BUFFER_SIZE;                     \
    dsize -= snprintf(dbuffer,dsize,"[%d]", node_address); \
    snprintf(dbuffer+(DEBUG_BUFFER_SIZE-dsize),dsize,arg); \
    printf("%s",dbuffer);                                  \
}
#define DEBUG_PID(pid,arg...){                                          \
    char dpbuffer[DEBUG_BUFFER_SIZE];                                   \
    int16_t dpsize = DEBUG_BUFFER_SIZE;                                 \
    dpsize -= snprintf(dpbuffer,dpsize,"[%d]",(pid));                   \
    dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,arg); \
    if(dpsize >= sizeof("[XXXXX]")){                                    \
        DEBUG("%s",dpbuffer);                                           \
    }                                                                   \
}
#define DEBUG_SHORT(arg...) {printf(arg);}

#endif
