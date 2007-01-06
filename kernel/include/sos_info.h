/**
 * @brief    header file for node id
 * @author   Simon Han
 */


#ifndef _SOS_INFO_H
#define _SOS_INFO_H

#include <sos_types.h>

#ifndef NO_SOS_UART
#include <uart.h>
#endif
#ifndef NO_SOS_I2C
#include <i2c_const.h>
#endif

/** Platform id's */
/* make sure to add and entry below so HW_TYPE is defined correctly */
enum {
	UNKNOWN = 0,
	MICA2 = 1,
	MICAZ = 2,
	XYZ = 3,
	CRICKET = 4,
	PROTOSB = 5,
	TMOTE = 6,
	CYCLOPS = 7,
	GW = 200,
	SIM = 201,
	PLATFORM_ANY  = 255,
};

/** MCU id's */
enum {
	MCU_UNKNOWN  = 0,
	MCU_AVR      = 1,
	MCU_ARM7     = 2,
	MCU_MSP430   = 3,
};

#if defined(MICA2_PLATFORM)
  #define HW_TYPE MICA2
  #define MCU_TYPE  MCU_AVR
#elif defined(MICAZ_PLATFORM)
  #define HW_TYPE MICAZ
  #define MCU_TYPE  MCU_AVR
#elif defined(XYZ_PLATFORM)
  #define HW_TYPE XYZ
  #define MCU_TYPE  MCU_ARM7
#elif defined(CRICKET_PLATFORM)
  #define HW_TYPE CRICKET
  #define MCU_TYPE  MCU_AVR
#elif defined(PROTOSB_PLATFORM)
  #define HW_TYPE PROTOSB
  #define MCU_TYPE  MCU_AVR
#elif defined(SOS_GW)
  #define HW_TYPE GW
  #define MCU_TYPE  MCU_UNKNOWN
#elif defined(SOS_SIM)
  #define HW_TYPE SIM
	#if defined(EMU_MICA2)
  #define MCU_TYPE  MCU_AVR
	#elif defined(EMU_XYZ)
  #define MCU_TYPE  MCU_ARM7
	#else
  #define MCU_TYPE  MCU_UNKNOWN
	#endif
#elif defined(CYCLOPS_PLATFORM)
  #define HW_TYPE CYCLOPS
  #define MCU_TYPE  MCU_AVR
#elif defined(TMOTE_PLATFORM)
  #define HW_TYPE TMOTE
  #define MCU_TYPE  MCU_MSP430
#else
  #define HW_TYPE UNKNOWN
  #define MCU_TYPE  MCU_UNKNOWN
#endif

#ifdef SUPPORTS_PACKED
#define PACK_STRUCT  __attribute__ ((packed))
#else
#define PACK_STRUCT
#endif

#ifdef _SOS_SERVER_CLIENT_
#define _SOS_SERVER_APP_
#endif


#ifndef SOS_NIC
#if defined(SOS_UART_NIC) || defined(SOS_I2C_NIC)
#define SOS_NIC
#endif
#else
#error "Please specify SOS_UART_NIC or SOS_I2C_NIC, SOS_NIC will automaticaly be set."
#endif


enum { UNIT_CENTIMETERS=0, UNIT_METERS=1, UNIT_KILOMETERS=2, UNIT_INCHES=3, UNIT_FEET=4, UNIT_MILES=5 };

enum { NORTH, SOUTH, WEST, EAST };

typedef struct gps_xy_type{
    int8_t dir;
    int8_t deg;
    int8_t min;
    int8_t sec;
} PACK_STRUCT
gps_xy_t;

/* 12 bytes */
typedef struct gps_type {
    gps_xy_t x;       /*  4 bytes  */
    gps_xy_t y;       /*  4 bytes  */
    int16_t unit;     /*  2 bytes  */
    int16_t z;        /*  2 bytes  */
} PACK_STRUCT
gps_t;

/* 8 bytes */
typedef struct node_loc_type {
	int16_t unit;
	int16_t x;
	int16_t y;
	int16_t z;
} PACK_STRUCT
node_loc_t;

#define BCAST_ADDRESS 0xFFFF

//GW_ADDR = 0xFFFE,

#define DEFAULT_MAX_MSG_LEN 0x80

// get node address
#ifdef NODE_ADDR
#define NODE_ADDRESS NODE_ADDR
#else
#if !defined(_MODULE_) && !defined(_SOS_SERVER_APP_)
#error ADDRESS must be defined on the command line! "make mica2 ADDRESS=2"
#endif
#endif
#define RADIO_MAX_MSG_LEN DEFAULT_MAX_MSG_LEN

// protect users from themselfs
#if defined(DISABLE_RADIO) && !defined(NO_SOS_RADIO)
#define NO_SOS_RADIO
#endif
#if defined(NO_SOS_RADIO) && !defined(NO_SOS_RADIO_MGR)
#define NO_SOS_RADIO_MGR
#endif


// get i2c address or use masked node address
#ifdef I2C_ADDR
#define I2C_ADDRESS I2C_ADDR
#else
#define I2C_ADDRESS (0x7f & NODE_ADDRESS)
#endif
#define I2C_MAX_MSG_LEN DEFAULT_MAX_MSG_LEN

// protect users from themselfs
#if defined(DISABLE_I2C) && !defined(NO_SOS_I2C)
#define NO_SOS_I2C
#endif
#if defined(NO_SOS_I2C) && !defined(NO_SOS_I2C_MGR)
#define NO_SOS_I2C_MGR
#endif

// get uart address or use default
#ifdef UART_ADDR
#define UART_ADDRESS UART_ADDR
#else
#define UART_ADDRESS 0xFFFD
#endif
#define UART_MAX_MSG_LEN DEFAULT_MAX_MSG_LEN

// protect users from themselfs
#if defined(DISABLE_UART) && !defined(NO_SOS_UART)
#define NO_SOS_UART
#endif
#if defined(NO_SOS_UART) && !defined(NO_SOS_UART_MGR)
#define NO_SOS_UART_MGR
#endif

#ifndef _MODULE_

#ifndef QUALNET_PLATFORM
extern uint16_t node_address;
extern uint8_t  node_group_id;
extern node_loc_t node_loc;
extern gps_t gps_loc;
extern uint8_t hw_type;

extern int8_t id_init(void);
extern uint16_t ker_id(void);
extern node_loc_t ker_loc(void);
extern gps_t ker_gps(void);
extern uint32_t ker_loc_r2(node_loc_t *loc1, node_loc_t *loc2);
extern uint16_t ker_uart_id(void);
extern uint8_t ker_i2c_id(void);
extern uint8_t ker_hw_type(void);

uint8_t ker_get_group(void);
void ker_set_group(uint8_t new_group_id);
void ker_set_id(uint16_t alloc_address);

#endif //QUALNET_PLATFORM

#endif /* _MODULE_ */

#endif /* _ID_H */

