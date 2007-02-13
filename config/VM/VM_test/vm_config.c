/** 
 * Static SOS programs must include <sos.h>
 */
#include <sos.h>

mod_header_ptr dvm_get_header();
//mod_header_ptr script_loader_get_header();

void sos_start(void)
{
  ker_register_module(dvm_get_header());
  //  ker_register_module(script_loader_get_header());
}
