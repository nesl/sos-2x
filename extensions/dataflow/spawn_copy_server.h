/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _SPAWN_SERVER_INCL_H_
#define _SPAWN_SERVER_INCL_H_

#include <sos.h>
#include <codemem.h>

mod_header_ptr copy_module_header(sos_code_id_t cid);
int8_t update_last_module_added(sos_pid_t pid);
int8_t delete_module_header(sos_pid_t mid);

#endif

