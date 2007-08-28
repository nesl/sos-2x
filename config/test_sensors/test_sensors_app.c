
#include <sos.h>

mod_header_ptr test_sensor_get_header();
mod_header_ptr par_sensor_get_header();
mod_header_ptr tsr_sensor_get_header();
mod_header_ptr sht1x_sensor_get_header();

/**
 * application start
 * This function is called once at the end od SOS initialization
 */
void sos_start(void)
{
	ker_register_module(par_sensor_get_header());
	ker_register_module(tsr_sensor_get_header());
	ker_register_module(sht1x_sensor_get_header());
	ker_register_module(test_sensor_get_header());
}
