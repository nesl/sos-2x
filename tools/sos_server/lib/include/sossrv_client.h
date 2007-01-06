/**
 * \file sossrv_client.h
 * \brief sossrv library API
 * \author Ram Kumar
 */

#include <pid.h>
#include <sos_inttypes.h>
#include <hardware_proc.h>
#include <message_types.h>


#define DEFAULT_IP_ADDR "127.0.0.1"
#define DEFAULT_PORT "7915"

/**
 * Prototype of the callback function
 *
 * \li Returns an integer.
 * \li Accepts an SOS Message as a parameter.
 */

typedef int (*recv_msg_func_t)(Message* msg);

/**
 * Setup a connection to SOS server
 *
 * \return 0 upon success
 *
 * This function will setup a TCP connection to the SOS server if it is up and running.
 * If a connection cannot be established, it will halt the execution of the program
 * and exit with an error message.
 * 
 */
int sossrv_connect(char* server_addr, char* server_port);

/**
 * \brief Disconnect from the SOS Server
 */
void sossrv_disconnect();

/**
 *  Post a message to the sensor network through the SOS server
 *
 * \param did Module ID of the message recepient
 * \param sid Module ID of the process sending the message
 * \param type Message type
 * \param length Length of the message payload in bytes
 * \param data Pointer to the message payload
 * \param saddr Source address of the base-station node
 * \param daddr Destination address of the node receiving the message
 *
 * \return 0 if the client is connected to the SOS server else returns -1
 *
 * This function will construct a properly formatted SOS message and 
 * dispatch it to the SOS server tool which would then forward it to the sensor network
 * through the gateway node.
 */
int sossrv_post_msg(sos_pid_t did, sos_pid_t sid, uint8_t type,
		    uint8_t length, void *data, uint16_t saddr, uint16_t daddr);

/**
 * Setup a callback for receiving messages
 * 
 * \param func Function pointer of the callback function
 *
 * \return 0 if the client is connected to the SOS server else returns -1
 *
 * This function sets up the callback function that would be invoked everytime a message from
 * the sensor network is forwarded to the client. The callback function has a fixed
 * prototype.
 */
int sossrv_recv_msg(recv_msg_func_t func);

