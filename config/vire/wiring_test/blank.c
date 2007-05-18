#include <sos.h>
#include <codemem.h>

//! forward declaration
mod_header_ptr loader_get_header();
mod_header_ptr wiring_engine_get_header();
mod_header_ptr spawn_copy_server_get_header();
mod_header_ptr source_get_header();
mod_header_ptr combine_get_header();
mod_header_ptr combine3_get_header();
mod_header_ptr transmit_get_header();
mod_header_ptr truncate_get_header();
mod_header_ptr truncate_long_get_header();
mod_header_ptr led_disp_get_header();
mod_header_ptr script_loader_get_header();
#ifdef USE_VIRE_TOKEN_MEM
mod_header_ptr vire_mem_server_get_header();
#endif

void sos_start(void){
	ker_register_module(loader_get_header());
	ker_register_module(wiring_engine_get_header());
	ker_register_module(spawn_copy_server_get_header());
#ifdef USE_VIRE_TOKEN_MEM
	ker_register_module(vire_mem_server_get_header());
#endif
	codemem_register_module(source_get_header());
	codemem_register_module(combine_get_header());
	codemem_register_module(combine3_get_header());
	codemem_register_module(transmit_get_header());
	codemem_register_module(truncate_get_header());
	codemem_register_module(truncate_long_get_header());
	codemem_register_module(led_disp_get_header());
	ker_register_module(script_loader_get_header());

}

