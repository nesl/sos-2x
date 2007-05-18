#include <sos.h>
#include <codemem.h>
#ifdef SOS_SIM
#include <sim_interface.h>
#endif

//---------------------------------------------------
// Statically compiled modules and Sim target modules
//---------------------------------------------------
mod_header_ptr loader_get_header();

mod_header_ptr frame_grabber_get_header();
mod_header_ptr update_background_get_header();
mod_header_ptr average_background_get_header();
mod_header_ptr absolute_subtract_get_header();
mod_header_ptr max_locate_get_header();
mod_header_ptr over_thresh_get_header();
mod_header_ptr select_transmit_get_header();
mod_header_ptr send_image_get_header();

mod_header_ptr wiring_engine_get_header();
mod_header_ptr spawn_copy_server_get_header();
#ifdef USE_VIRE_TOKEN_MEM
mod_header_ptr vire_mem_server_get_header();
#endif

/**
 * application start
 * This function is called once at the end of SOS initialization
 */
void sos_start(void)
{
  codemem_register_module(frame_grabber_get_header());
  codemem_register_module(update_background_get_header());
  codemem_register_module(average_background_get_header());
  codemem_register_module(absolute_subtract_get_header());
  codemem_register_module(max_locate_get_header());
  codemem_register_module(over_thresh_get_header());
  codemem_register_module(select_transmit_get_header());
  codemem_register_module(send_image_get_header());

  ker_register_module(loader_get_header());
  ker_register_module(wiring_engine_get_header());
  ker_register_module(spawn_copy_server_get_header());
#ifdef USE_VIRE_TOKEN_MEM
  ker_register_module(vire_mem_server_get_header());
#endif

#ifdef SOS_SIM
  module_headers[0] = frame_grabber_get_header();
  module_headers[1] = update_background_get_header();
  module_headers[2] = average_background_get_header();
  module_headers[3] = absolute_subtract_get_header();
  module_headers[4] = max_locate_get_header();
  module_headers[5] = over_thresh_get_header();
  module_headers[7] = select_transmit_get_header();
#endif
}

