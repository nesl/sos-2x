/** @file sim_interface.c
 * @brief sim_specific files in order to simulate module loading.
 * @author Nicholas Kottenstette
 * Academic Free License, Version 1.2
 * This is open source.  For exact license terms read the LICENSE file
 * Copyright 2005, Nicholas Kottenstette. All Rights Reserved.
 */
#include <hardware.h>
#include <sos_info.h>
#include "sim_interface.h"

#ifndef SOS_DEBUG_SIM_INTERFACE
#undef DEBUG
#define DEBUG(...)
#endif

mod_header_ptr module_headers[MAX_NETWORK_MODULES];
mod_header_ptr get_header_from_sim(sos_code_id_t cid)
{
    int i;
    DEBUG("get_header_from_sim(cid: %d)\n", cid);
    for(i = 0; i < MAX_NETWORK_MODULES; i++){
        mod_header_ptr h;
        sos_code_id_t code_id;
        h = module_headers[i];
		if(h == 0) continue;
        DEBUG("module_headers[%d] = 0x%x\n", i, h);
        code_id = sos_read_header_word(h, offsetof(mod_header_t, code_id));
		code_id = entohs( code_id );

        DEBUG("code_id = %d\n", code_id);
        if(cid == code_id){
            DEBUG("cid(%d) == code_id(%d)\n",cid,code_id);
            return h;
        }
    }
    DEBUG("get_header_from_sim no header found\n");
    return (mod_header_ptr)NULL;
}

#if 0
void set_version_to_sim(sos_pid_t pid, uint8_t version)
{
    int i;
    for(i = 0; i < MAX_NETWORK_MODULES; i++){
        mod_header_ptr h;
        sos_pid_t mod_id;
        
        h = module_headers[i];
        if(h == 0) continue;
        mod_id = sos_read_header_byte(h, offsetof(mod_header_t, mod_id));
        
        if(pid == mod_id){
            mod_header_t *header_ptr = (mod_header_t *) h;
            header_ptr->version = version;	
            return;
        }
    }
}
#endif
