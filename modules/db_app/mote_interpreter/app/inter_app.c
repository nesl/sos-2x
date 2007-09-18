#include <sos.h>

mod_header_ptr tree_routing_get_header();
mod_header_ptr neighbor_get_header();
mod_header_ptr accel_sensor_get_header();
mod_header_ptr interpreter_get_header();

mod_header_ptr loader_get_header();
void sos_start(void){
    ker_register_module(loader_get_header());
    ker_register_module(neighbor_get_header());
    ker_register_module(tree_routing_get_header());
    ker_register_module(accel_sensor_get_header());
    ker_register_module(interpreter_get_header());
}

