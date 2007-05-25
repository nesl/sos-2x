/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the MDA300 Sensor Board
 * 
 * This module currently provides support for measuring
 * temperature and humidity from the SHT15 chip.
 * 
 * \author Kapy Kangombe, John Hicks, James Segedy {jsegedy@gmail.com}
 * \date 07-2005
 * Ported driver from TinyOS to SOS
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * \date 05-2007
 * Ported driver to current version of SOS
 */

#include <sys_module.h>
#include "sht15.h"
#include <led_dbg.h>

////
// Hardware specific defines
//
// These may be moved to a driver file later on in the future.
////
//                                   adr command  r/w
#define STATUS_REG_WRITE    0x06  // 000   0011    0
#define STATUS_REG_READ     0x07  // 000   0011    1
#define MEASURE_TEMPERATURE 0x03  // 000   0001    1
#define MEASURE_HUMIDITY    0x05  // 000   0010    1
#define RESET               0x1e  // 000   1111    0


/** 
 * SHT15 data and clock line operations
 *
 * \note The SHT15 uses a modified version of the I2C interface.
 *
 * \todo Move some platform specific pin definitions into an external platform
 * include file.  Continue by udating this driver to use the SHIT15_* pin and
 * port defines rather than direct pin and port defines.
 */

#define SHT15_PORT PORTE
#define SHT15_DIRECTION DDRE
#define SHT15_READ_PIN PINE
#define SHT15_PIN_NUMBER 7
#define SHT15_PIN 0x80

#define set_data()          PORTE |=  (1<<PE7)
#define clear_data()        PORTE &= ~(1<<PE7)
#define set_clock()         PORTC |=  (1<<PC0)
#define clear_clock()       PORTC &= ~(1<<PC0)
#define make_data_output()  DDRE  |=  (1<<DDE7)
#define make_data_input()   DDRE  &= ~(1<<DDE7)
#define make_clock_output() DDRC  |=  (1<<DDC0)



////
// Local enums
////

/**
 * Type of reading that is requisted from the SHT15
 */
enum sht15_command
{
    TEMPERATURE, 
    HUMIDITY,
};


/**
 * Ack flag
 */
enum sht15_ack
{
    ACK,
    NOACK,
};


/**
 * Local timer
 */
enum
{
    SHT15_TEMPERATURE_TIMER,
    SHT15_HUMIDITY_TIMER,
};

#define SHT15_TEMPERATURE_TIME 230L
#define SHT15_HUMIDITY_TIME 70L

// SOS module specifc funictions and state

static int8_t sht15_msg_handler(void *start, Message *e);

typedef struct 
{
    uint8_t calling_module;
} sht15_state_t;

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = SHT15_ID,
    .state_size     = sizeof(sht15_state_t),
    .num_sub_func   = 0,
    .num_prov_func  = 0,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(SHT15_ID),
    .module_handler = sht15_msg_handler,
};


//// 
// Local function prototypes
////

static void delay();

static bool write_byte(uint8_t value);

static uint8_t read_byte(enum sht15_ack ack);

static void start_transmission();

static void connection_reset();

static bool measure(enum sht15_command command);

static bool get_data(uint8_t *data, uint8_t *checksum, enum sht15_command command);


////
// Code, glorious code!
////

/** Main message handler for SHT15
 *
 * \param state ID of the last calling module 
 *
 * \param msg Incoming message
 *
 * This kernel module handles the following mellage types from other
 * applications:
 *
 * \li SHT15_GET_TEMPERATURE Generates a SHT15_TEMPERATURE message that is sent
 * to the calling applications and contains the temperature on the sht15 chip.
 *
 * \li SHT15_GET_HUMIDITY Generates a SHT15_HUMIDITY message that is sent to
 * the calling application and contains the humidity on the sht15 chip.
 *
 * All of these messages return SOS_OK to the kernel if all goes well.  An
 * error code is returned if something goes wrong.  Perhaps you could have
 * guessed that.
 *
 */
static int8_t sht15_msg_handler(void *state, Message *msg) { 

    sht15_state_t *s = (sht15_state_t *)state;

    /** <h2>Types of messages handled by the SHT15 module</h2>
    */
    switch(msg->type)
    {
        /**
         * \par MSG_INIT
         * Start up the SHT15.  This is easy, since the SHT15 started when the
         * mote was powerd on.
         */

        case MSG_INIT:
            { 
                s->calling_module = 0;
                
                sys_led(LED_RED_ON);
                sys_led(LED_GREEN_ON);
                sys_led(LED_YELLOW_ON);
                break; 
            }


            /** 
             * \par MSG_FINAL
             * Stick a fork in it, cause this module is done.
             */    
        case MSG_FINAL:
            { 
                sys_led(LED_RED_OFF);
                sys_led(LED_GREEN_OFF);
                sys_led(LED_YELLOW_OFF);
                break; 
            }   


            /** 
             * Grab data from the SHT15
             */
        case MSG_TIMER_TIMEOUT:
            {
                uint8_t checksum;
                uint8_t *data;
                bool no_error;

                data = (uint8_t *)sys_malloc(2);
                
                MsgParam *param = (MsgParam*) (msg->data);
                switch (param->byte) {

                    case SHT15_TEMPERATURE_TIMER:
                        no_error = get_data(data, &checksum, TEMPERATURE);
                        if (no_error == false) 
                        {
                            data[0] = 0xFF;
                            data[1] = 0xFF;
                            connection_reset();
                        }
                        sys_led(LED_GREEN_TOGGLE);
                        sys_post(s->calling_module, 
                                SHT15_TEMPERATURE, 
                                2, 
                                data, 
                                SOS_MSG_RELEASE);
                        break;

                    case SHT15_HUMIDITY_TIMER:
                        no_error = get_data(data, &checksum, HUMIDITY);
                        if (no_error == false) 
                        {
                            data[0] = 0xFF;
                            data[1] = 0xFF;
                            connection_reset();
                        }
                        sys_led(LED_GREEN_TOGGLE);
                        sys_post(s->calling_module, 
                                SHT15_HUMIDITY, 
                                2, 
                                data, 
                                SOS_MSG_RELEASE);
                        break;

                    default:
                        sys_free(data);
                        break;

                }
                break;
            }


            /**
             * \par SHT15_GET_TEMPERATURE or SHT15_GET_HUMIDITY
             * Request a temperature or humidity reading from the SHT15.
             * 
             * \par
             * \todo Add in check of checksum?
             */
        case SHT15_GET_TEMPERATURE:
        case SHT15_GET_HUMIDITY:
            {  
                bool no_error;

                s->calling_module = msg->sid;
                
                // Connect to the SHT15
                /** 
                 * \todo Is a rest neeeded before talking to the sht15?  Once
                 * the code is up and running, look into removing it.
                 */
                make_clock_output();
                connection_reset(); 

                // Take a measruement.  If an error occures reset the SHT15
                // and send 0xFFFF as the "data" to the calling module.
                if(msg->type == SHT15_GET_TEMPERATURE) {
                    no_error = measure(TEMPERATURE);
                } else if (msg->type == SHT15_GET_HUMIDITY) {
                    no_error = measure(HUMIDITY);
                } else {
                    sys_led(LED_RED_TOGGLE);
                    sys_led(LED_GREEN_TOGGLE);
                    sys_led(LED_YELLOW_TOGGLE);
                    return SOS_OK;
                    //return -EINVAL;
                }

                if (no_error == false) 
                {
                    connection_reset();
                }

                break;
            }    


            /**
             * \par default
             * Unknowen message type.  Bummer.
             */
        default:
            {
                return -EINVAL;
            }      

    }

    return SOS_OK;

}


////
// Implementation of local functions
////


/**
 * Hardware specific delay.  This is target towards the mica2 and micaZ motes.
 * 
 * These will probably be moved into a driver file at some point, since this
 * is hardware specific.
 */
static void delay() {
    asm volatile  ("nop" ::);
    asm volatile  ("nop" ::);
    asm volatile  ("nop" ::);
}


/** 
 * Write a byte to the SHT15
 *
 * \param byte Byte to write to the SHT15
 *
 * \return True if there were no errors
 *
 */
static bool write_byte(uint8_t byte)
{
    HAS_CRITICAL_SECTION;

    uint8_t i;

    ENTER_CRITICAL_SECTION();
    for (i=0x80; i>0; i = i>>1) 
    {
        if (i & byte) {
            set_data();
        } else {
            clear_data();
        }

        set_clock();
        delay();
        delay();
        clear_clock();        
    }

    set_data();
    make_data_input();
    delay();
    set_clock();

    // SHT15 pulls line low to signal ACK so 0 is good
    if ((SHT15_READ_PIN & (SHT15_PIN)) != 0) {
        return false;
    }

    delay();
    clear_clock();
    make_data_input();
    
    LEAVE_CRITICAL_SECTION();
    return true;
}


/** 
 * Reads a byte from the SHT15 
 *
 * \param ack Enable or disable ack for the byte
 *
 * \return Byte read from the SHT15
 *
 */
static uint8_t read_byte(enum sht15_ack ack)
{
    HAS_CRITICAL_SECTION;

    uint8_t i;
    uint8_t val;

    val = 0;

    ENTER_CRITICAL_SECTION();

    set_data();
    make_data_input();
    delay();

    for (i=0x80; i>0; i = i>>1) 
    {
        set_clock();
        delay();
    
        // SHT15 pulls line low to signal 0, otherwise, we need to put on one
        // into the correct bit of val
        if ((SHT15_READ_PIN & (SHT15_PIN)) != 0) {
            val = (val | i);
        }
        clear_clock();
    }

    make_data_output();

    if (ack == ACK) {
        clear_data();
    }

    delay();
    set_clock();
    delay();
    clear_clock();
    LEAVE_CRITICAL_SECTION();

    return val;
}

/** 
 * Generates a transmission start
 * 
 * The lines should look like:
 *       ____      _____
 * DATA:     |____|
 *          __    __
 * SCK : __|  |__|  |____
 */
static void start_transmission() 
{
    HAS_CRITICAL_SECTION;
    ENTER_CRITICAL_SECTION();
    make_data_output();
    set_data(); 
    clear_clock();
    delay();
    set_clock();
    delay();
    clear_data();
    delay();
    clear_clock();
    delay();
    set_clock();
    delay();
    set_data();
    delay();
    clear_clock();
    LEAVE_CRITICAL_SECTION();
}


/**
 * Reset a connection if anything should go wrong
 */
static void connection_reset() 
{
    HAS_CRITICAL_SECTION;

    uint8_t i;

    ENTER_CRITICAL_SECTION();
    make_data_output();
    set_data(); 
    clear_clock();
    for (i=0; i<9; i++) 
    {
        set_clock();
        delay();
        clear_clock();
    }
    LEAVE_CRITICAL_SECTION();
}


/** 
 * Begin a measurement of either temperature or humidity from the SHT15
 *
 * \param command Either TEMPERATURE to request a temperature measurement or
 * HUMIDITY to measure humidity
 *
 * \return True if there were no errors
 * 
 */
static bool measure(enum sht15_command command) 
{
    bool no_error;

    start_transmission();
    switch (command) 
    {
        case TEMPERATURE: 
            no_error = write_byte(MEASURE_TEMPERATURE); 
            if(no_error == false) {
                sys_led(LED_RED_TOGGLE);
                return false;
            }
            sys_timer_start(SHT15_TEMPERATURE_TIMER, SHT15_TEMPERATURE_TIME, TIMER_ONE_SHOT);
            break;

        case HUMIDITY: 
            no_error = write_byte(MEASURE_HUMIDITY); 
            if(no_error == false) {
                sys_led(LED_RED_TOGGLE);
                return false;
            }
            sys_timer_start(SHT15_HUMIDITY_TIMER, SHT15_HUMIDITY_TIME, TIMER_ONE_SHOT);
            break;

        default:   
            return false;
    }

    return true;
}


/** 
 * Get the measurement from the SHT15
 *
 * \param data Pointer to a 2-byte data buffer
 *
 * \param checksum Pointer to a 1-byte checksum
 *
 * \param command Either TEMPERATURE to request a temperature measurement or
 * HUMIDITY to measure humidity
 *
 * \return True if there were no errors
 *
 * \note The timing used to delay calls to this function are dependent on the
 * measurement size being requested from the SHT15.  This code assumes the
 * defaults of 12-bit temperature readings and 14-bit humidity readings.
 * 
 */
static bool get_data(uint8_t *data, uint8_t *checksum, enum sht15_command command)
{
    
    HAS_CRITICAL_SECTION;

    make_data_input();
    delay();
    
    // SHT15 pulls line low to signal that the data is ready
    if ((SHT15_READ_PIN & (SHT15_PIN)) != 0) {
        return false;
    }

    data[0] = read_byte(ACK);
    data[1] = read_byte(ACK);
    *checksum  = read_byte(NOACK);
    
    ENTER_CRITICAL_SECTION();    
    LEAVE_CRITICAL_SECTION();
        
    return true;
}

#ifndef _MODULE_
mod_header_ptr sht15_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif



