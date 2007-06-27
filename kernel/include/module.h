/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
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
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
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
/** 
 * @brief Header file for modules
 * 
 * This is the only file that module writer should include.
 * Currently, we do not support PC emulation on this.
 */

#ifndef _MODULE_H
#define _MODULE_H

#ifndef _MODULE_
#include <sos.h>
#include <sos_sched.h>
// Ram - I am including the following file here because the configuration
// needs to know what sensor board it is being compiled to
#include <sensor.h>
#include <sos_error_types.h>
#include <random.h>
#endif

#include <sos_info.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <monitor.h>
#include <message_types.h>
#include <codemem.h>
#include <systime.h>
#include <sos_shm.h>
#include <pid.h>
#include <stddef.h>
#include <kertable_conf.h>
#include <fntable_types.h>
#include <sos_error_types.h>
#ifdef SOS_SFI
#include <memmap.h>
#endif
// Ram - I have removed the following include file
// Module writers interacting with sensorboards
// should specify the header file for that board
// explicitly in their files
//#include <sensor.h>

#ifdef _MODULE_
#error "Loadable module cannot include module.h"
#endif /* #ifdef _MODULE_ */
#endif /* #ifndef _MODULE_H */

