#include <sos.h>
#ifdef SOS_SIM
#include <sim_interface.h>
mod_header_ptr blink_get_header();
mod_header_ptr surge_get_header();
mod_header_ptr tree_routing_get_header();
mod_header_ptr test_param_get_header();
#endif

//! forward declaration
mod_header_ptr loader_get_header();


void sos_start(void){
	ker_register_module(loader_get_header());

#ifdef SOS_SIM
	module_headers[0] = blink_get_header();
	module_headers[1] = surge_get_header();
	module_headers[2] = tree_routing_get_header();
	module_headers[3] = test_param_get_header();
	

	/*
	 * Upto MAX_NETWORK_MODULES header can be defined.
	 * MAX_NETWORK_MODULES is defined in platform/sim/include/sim_interface.h 
	 */
	//module_headers[MAX_NETWORK_MODULES] = ...
#endif
}

