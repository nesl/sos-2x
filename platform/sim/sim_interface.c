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
#include <dlfcn.h>

#ifndef SOS_DEBUG_SIM_INTERFACE
#undef DEBUG
#define DEBUG(...)
#endif

mod_header_ptr module_headers[MAX_NETWORK_MODULES];
static void * dlfd[65536] = {NULL};

mod_header_ptr get_header_from_sim(sos_code_id_t cid)
{
    int i;
	char name_buf[128];
	void *dlh;
	void *mod_sym;
	int cnt;
				
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
	if( dlfd[cid] != NULL ) {
		dlh = dlfd[cid];
	} else {
		DEBUG("try look for %d.sos\n", cid);
		cnt = sprintf(name_buf, "./%d.sos", cid);
		name_buf[cnt] = '\0';
		dlh = dlopen(name_buf, RTLD_NOW | RTLD_LOCAL );
		if( dlh == NULL ) {
			DEBUG("cannot find %d.sos\n", cid);
			DEBUG("dlerror: %s\n", dlerror());
			return (mod_header_ptr)NULL;
		}
		if( dlsym( dlh, "mod_header" ) != NULL ) {
			dlfd[cid] = dlh;
		} else {
			DEBUG("dlerror: %s\n", dlerror());
			return (mod_header_ptr)NULL;
		}
		DEBUG("found %d.sos\n", cid);	
	}
	mod_sym = dlsym( dlh, "mod_header" );
	return (mod_header_ptr)mod_sym;
}

void delete_module_image( sos_code_id_t cid )
{
	if( dlfd[cid] != NULL ) {
		DEBUG("sim closing cid %d\n", cid);
		dlclose(dlfd[cid]);
		dlfd[cid] = NULL;
	}
}

