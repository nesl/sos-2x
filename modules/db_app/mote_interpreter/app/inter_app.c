#include <sos.h>

mod_header_ptr neighbor_get_header();
mod_header_ptr accel_sensor_get_header();

#ifdef SOS_SIM
  mod_header_ptr interpreter_get_header();
  mod_header_ptr tree_routing_get_header();
#else
//  mod_header_ptr mag_sensor_get_header();
//  mod_header_ptr phototemp_sensor_get_header();
#endif

mod_header_ptr loader_get_header();
void sos_start(void){
    ker_register_module(loader_get_header());
    ker_register_module(neighbor_get_header());
    ker_register_module(accel_sensor_get_header());
#ifdef SOS_SIM
    ker_register_module(interpreter_get_header());
    ker_register_module(tree_routing_get_header());
#else
  //  ker_register_module(mag_sensor_get_header());
   // ker_register_module(phototemp_sensor_get_header());
#endif
}

