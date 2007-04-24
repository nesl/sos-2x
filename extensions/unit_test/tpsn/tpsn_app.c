#include <sos.h>

mod_header_ptr tpsn_get_header();
mod_header_ptr test_tpsn_get_header();
void sos_start(void){
  ker_register_module(tpsn_get_header());
  ker_register_module(test_tpsn_get_header());
}
