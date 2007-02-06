/** 
 * Static SOS programs must include <sos.h>
 */
#include <sos.h>

mod_header_ptr loader_get_header();

void sos_start(void)
{
 ker_register_module(loader_get_header());
}
