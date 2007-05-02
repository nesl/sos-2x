#ifndef _SOS_RADIO_H
#define _SOS_RADIO_H

/**
 * @brief radio 
 */
extern void radio_init();
extern void radio_final();
extern void radio_msg_alloc(Message *e);
extern int8_t radio_set_timestamp(bool on);
extern void radio_gc(void);
extern void radio_msg_gc(void);



#endif // _SOS_RADIO_H

