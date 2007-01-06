/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


#include <sos.h>
#include <sos_inttypes.h>
#include <sos_sched.h>
#include <mod_pid.h>
#include <fntable.h>
#include <sensor.h>
#include <adc_proc.h>
#include "photo.h"

#if 0
enum {
	PHOTO_GET_DATA_FID = 0,
	PHOTO_SENSOR_PID   = 128,
	PHOTO = 1,
};

typedef struct {
uint8_t mod_pid;
} photosensor_t;

static int8_t module(void *state, Message *msg);
static int8_t photo_get_data(func_cb_t *cb);

static mod_header_t mod_header SOS_MODULE_HEADER =
{
	mod_id : PHOTO_SENSOR_PID,
	state_size : sizeof(photosensor_t),
	num_sub_func : 0,
	num_prov_func : 1,
	module_handler: module,
	funct : {
				{photo_get_data, "cvv0", PHOTO_SENSOR_PID, PHOTO_GET_DATA_FID},
			},
};



static int8_t module(void *state, Message *msg)
{
  photosensor_t *s = (photosensor_t*)state;
  switch (msg->type) {
  case MSG_DATA_READY:
	{
	  MsgParam *p = (MsgParam*)(msg->data);
	  ker_sensor_data_ready(PHOTO, p->word);
	  return SOS_OK;
	}
  case MSG_INIT:
	{
	  ker_adc_proc_bindPort(TOS_ADC_LIGHT_PORT, TOSH_ACTUAL_LIGHT_PORT, msg->did);
	  ker_sensor_register(msg->did, PHOTO, PHOTO_GET_DATA_FID);
	  s->mod_pid = msg->did;
	  return SOS_OK;
	}
  case MSG_FINAL:
	{
	  ker_sensor_deregister(msg->did, PHOTO, PHOTO_GET_DATA_FID);
	  return SOS_OK;
	}
  default:
	return -EINVAL;
  }
  return SOS_OK;
}

static int8_t photo_get_data(func_cb_t *cb)
{
  return ker_adc_proc_getData(TOS_ADC_LIGHT_PORT);
}


#endif
int8_t photosensor_init()
{
  //return ker_register_module(sos_get_header_address(mod_header));
	return 0;
}

