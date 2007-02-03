#include <sos.h>

mod_header_ptr rats_get_header();
mod_header_ptr loader_get_header();

void sos_start()
{
  ker_register_module(rats_get_header());
  ker_register_module(loader_get_header());
}

