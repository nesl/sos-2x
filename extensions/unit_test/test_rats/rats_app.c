#include <sos.h>
#include <led.h>

mod_header_ptr rats_get_header();
mod_header_ptr test_rats_get_header();

void sos_start()
{
	ker_register_module(rats_get_header());
	ker_register_module(test_rats_get_header());	
}
