
#ifndef _MODULE_VIRTUAL_H
#define _MODULE_VIRTUAL_H
/**
   * @brief default function error handlers
    */
//#define error_stub NULL
static inline void error_v(func_cb_ptr p)        
{
}

static inline int8_t error_8(func_cb_ptr p)
{
	    return -1;
}

static inline int16_t error_16(func_cb_ptr p)
{
	    return -1;
}

static inline int32_t error_32(func_cb_ptr p)
{   
	    return -1;
}   

static inline void* error_ptr(func_cb_ptr p)
{
	    return NULL;
}   


typedef int8_t (* post_link_func_t)(  sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *  larg, uint16_t flag, uint16_t daddr );

static inline int8_t post_net(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr)
{                                                             
	post_link_func_t func = (post_link_func_t)get_kertable_entry(25);
	return func(did, sid, type, arg, larg, flag|SOS_MSG_RADIO_IO, daddr);     
}                                                             

static inline int8_t post_auto(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr)            
{
	post_link_func_t func = (post_link_func_t)get_kertable_entry(25);
	return func(did, sid, type, arg, larg, flag | SOS_MSG_ALL_LINK_IO | SOS_MSG_LINK_AUTO, daddr);
}                                                             

static inline int8_t post_uart(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr)            
{                                                             
	post_link_func_t func = (post_link_func_t)get_kertable_entry(25);         
	return func(did, sid, type, arg, larg, flag|SOS_MSG_UART_IO, daddr);
}                                                             

static inline int8_t post_i2c(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr)             
{                                                             
	post_link_func_t func = (post_link_func_t)get_kertable_entry(25);         
	return func(did, sid, type, arg, larg, flag|SOS_MSG_I2C_IO, daddr);       
}                                                             

static inline int8_t post_spi(sos_pid_t did, sos_pid_t sid, uint8_t type, uint8_t arg, void *larg, uint16_t flag, uint16_t daddr)
{
	post_link_func_t func = (post_link_func_t)get_kertable_entry(25);         
	return func(did, sid, type, arg, larg, flag|SOS_MSG_SPI_IO, daddr);       
}        


#endif
