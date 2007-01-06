/**
 * @file dev_network.h
 * @brief Open a TCP connection to the sensor network
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _DEV_NETWORK_H_
#define _DEV_NETWORK_H_

/**
 * @brief Open a TCP connection to the sensor network
 * @param nicip <IP Address:Port Number> of the sensor network
 * @return 0 if successful, Exit(-1) on failure
 */
int open_network_device(char* nicip);

#endif//_DEV_NETWORK_H_
