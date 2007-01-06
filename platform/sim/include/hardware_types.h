
#ifndef _SOS_HARDWARD_TYPES_H_
#define _SOS_HARDWARD_TYPES_H_

#include <hardware_proc.h>
#include <sos_info.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

extern int debug_socket;
extern struct sockaddr_in debug_addr;

/*
 * runtime defines
 */
#ifndef SIM_PORT_OFFSET
#define SIM_PORT_OFFSET 20000 
#endif
#ifndef SIM_MAX_GROUP_ID
#define SIM_MAX_GROUP_ID 31
#endif
#ifndef SIM_MAX_MOTE_ID
#define SIM_MAX_MOTE_ID 1023
#endif
#ifndef SIM_DEBUG_PORT
#define SIM_DEBUG_PORT (SIM_PORT_OFFSET - 1)
#endif
/**
 *
 */
#define DEBUG_BUFFER_SIZE 1024
#if defined (__CYGWIN__) || !defined (DEBUG_XML_OUTPUT)
#define DEBUG(arg...){                                     \
    char dbuffer[DEBUG_BUFFER_SIZE];                       \
    int16_t dsize = DEBUG_BUFFER_SIZE;                     \
    dsize -= snprintf(dbuffer,dsize,"[%3d][%3d] ", node_address, ker_get_current_pid()); \
    snprintf(dbuffer+(DEBUG_BUFFER_SIZE-dsize),dsize,arg); \
    printf("%s",dbuffer);                                  \
}
#define DEBUG_PID(pid,arg...){                                          \
    char dpbuffer[DEBUG_BUFFER_SIZE];                                   \
    int16_t dpsize = DEBUG_BUFFER_SIZE;                                 \
    dpsize -= snprintf(dpbuffer,dpsize,"[%3d][%3d] ", node_address, (pid));  \
    dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,arg); \
    /*if(dpsize >= sizeof("[XXXXX]")){ */                                   \
    /*    DEBUG("%s",dpbuffer);        */                                   \
    /*}                                */                                   \
}
#else
#define DEBUG(arg...){                                                     \
    char dbuffer[DEBUG_BUFFER_SIZE];                                       \
    int16_t dsize = DEBUG_BUFFER_SIZE;                                     \
    dsize -= snprintf(dbuffer,dsize,"<debug address=%d>\n", node_address); \
    dsize -= snprintf(dbuffer+(DEBUG_BUFFER_SIZE-dsize),dsize,arg);        \
    if(dsize >= sizeof("</debug>\n")){                                     \
        dsize -= snprintf(dbuffer+(DEBUG_BUFFER_SIZE-dsize),dsize,"</debug>\n");\
        sendto(debug_socket, dbuffer, DEBUG_BUFFER_SIZE-dsize, MSG_DONTWAIT, (struct sockaddr *)&debug_addr, sizeof(struct sockaddr_in));                    \
    }                                                                      \
}
#define DEBUG_PID(pid,arg...){                                             \
    char dpbuffer[DEBUG_BUFFER_SIZE];                                      \
    int16_t dpsize = DEBUG_BUFFER_SIZE;                                    \
    dpsize -= snprintf(dpbuffer,dpsize,"<module pid=%d ",(pid));           \
    if((pid) <= KER_MOD_MAX_PID)                                           \
        dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,     \
                           "pid_name=\"%s\">\n", ker_pid_name[(pid)]);     \
    else if((pid) < APP_MOD_MIN_PID)                                       \
        dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,     \
                           "pid_name=\"device_%d\">\n",(pid)-DEV_MOD_MIN_PID); \
    else                                                                   \
        dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,     \
                           "pid_name=\"%s\">\n", mod_pid_name[(pid)-APP_MOD_MIN_PID]);                                                                     \
    dpsize -= snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,arg);    \
    if(dpsize >= sizeof("</module>\n<debug address=XXXXX>\n</debug>\n")){  \
        snprintf(dpbuffer+(DEBUG_BUFFER_SIZE-dpsize),dpsize,"</module>\n");\
        DEBUG("%s",dpbuffer);                                              \
    }                                                                      \
}
#endif

#define DEBUG_SHORT(arg...) {printf(arg);}

#endif

