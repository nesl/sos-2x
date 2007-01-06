/** @file sim_interface.h
 * @brief header file for sim_interface.c
 * @author Nicholas Kottenstette
 * Academic Free License, Version 1.2
 * This is open source.  For exact license terms read the LICENSE file
 * Copyright 2005, Nicholas Kottenstette. All Rights Reserved.
 */
#ifndef _SIM_H
#define _SIM_H
#define MAX_NETWORK_MODULES 0x80

extern void sim_module_header_init();
extern mod_header_ptr module_headers[];
extern mod_header_ptr get_header_from_sim(sos_code_id_t cid);
// extern void set_version_to_sim(sos_pid_t pid, uint8_t version);

#endif
