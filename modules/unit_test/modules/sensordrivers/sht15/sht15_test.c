/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 *
 */

/**
 * Test the sht15 kernel module
 * 
 * \author Roy Shea
 * \date 6/06
 * \date 5/07
 */

#include <sys_module.h>
#include <sensordrivers/sht15/sht15.h>
#include <led_dbg.h>

////
// Local enumerations
////

/**
 * Current functionality of the SHT15 kernel module that is being tested
 */
enum test_state {
    UNIT_SHT15_GET_TEMPERATURE = 0,
    UNIT_SHT15_GET_HUMIDITY = 1,
};


/**
 * Single timer used to iterativly trigger testing of functionality of the
 * SHT15
 */
enum {
    UNIT_SHT15_TIMER,
};



////
// Module specific state and headers
////

#define UNIT_SHT15_ID (SHT15_ID + 1)
#define UNIT_SHT15_TIMER_PERIOD 3000


/**
 * Module specific state used to track the current functionality of the SHT15
 * driver being testied
 */
typedef struct {
    enum test_state state;
} app_state_t;


static int8_t sht15_test_msg_handler(void *start, Message *e);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = UNIT_SHT15_ID,
    .state_size     = sizeof(app_state_t),
    .num_sub_func   = 0,
    .num_prov_func  = 0,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(UNIT_SHT15_ID),
    .module_handler = sht15_test_msg_handler,
};



////
// Code!  We love code!
////


/** Base message handler
 *
 * This module uses the driver to pull each of these forms of data from the
 * SHT15.  A new form of data is collected about UNIT_SHT15_TIMER_PERIOD ms.
 */
static int8_t sht15_test_msg_handler(void *state, Message *msg) {

    void *data;
    uint8_t length;
    app_state_t *s = (app_state_t *)state;
    
    switch (msg->type){

        case MSG_INIT:
            {
                
                sys_timer_start(UNIT_SHT15_TIMER, UNIT_SHT15_TIMER_PERIOD, TIMER_REPEAT);
                s->state = UNIT_SHT15_GET_TEMPERATURE;

                break;
            }


        case MSG_FINAL:
            {
                sys_timer_stop(UNIT_SHT15_TIMER);
                break;
            }


        case MSG_TIMER_TIMEOUT:
            {
                sys_led(LED_RED_TOGGLE);
                switch (s->state) {

                    case UNIT_SHT15_GET_TEMPERATURE:
                        sys_post_value(SHT15_ID, SHT15_GET_TEMPERATURE, 0, 0);
                        s->state = UNIT_SHT15_GET_HUMIDITY;
                        break;

                    case UNIT_SHT15_GET_HUMIDITY:
                        sys_post_value(SHT15_ID, SHT15_GET_HUMIDITY, 0, 0);
                        s->state = UNIT_SHT15_GET_TEMPERATURE;
                        break;

                    default:
                        s->state = UNIT_SHT15_GET_TEMPERATURE;
                        break;
                
                }
                break;
            }

        case SHT15_TEMPERATURE:
            
            sys_led(LED_YELLOW_TOGGLE);
            length = msg->len;
            data = sys_msg_take_data(msg);
            sys_post_uart(UNIT_SHT15_ID, SHT15_TEMPERATURE,
                    length, data, SOS_MSG_RELEASE, BCAST_ADDRESS);
            break;

        case SHT15_HUMIDITY:
            
            sys_led(LED_YELLOW_TOGGLE);
            length = msg->len;
            data = sys_msg_take_data(msg);
            sys_post_uart(UNIT_SHT15_ID, SHT15_HUMIDITY,
                    length, data, SOS_MSG_RELEASE, BCAST_ADDRESS);
            break;

        default:
            return -EINVAL;
    }


    return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr sht15_test_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif



