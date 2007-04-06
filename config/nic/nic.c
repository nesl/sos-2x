/**
 * \file nic_base.c
 * \brief This application is the SOS NIC.
 */
#include <sos.h>

mod_header_ptr nic_get_header();

void sos_start(void)
{
	ker_register_module(nic_get_header());

}
