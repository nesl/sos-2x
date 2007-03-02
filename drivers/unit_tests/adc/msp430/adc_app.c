#include <sos.h>
mod_header_ptr adc_get_header();
mod_header_ptr adc_sampler_get_header();

void sos_start(void)
{
    ker_register_module(adc_get_header());
    ker_register_module(adc_sampler_get_header());
}

