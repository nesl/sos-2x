/* -*-C-*- */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dev_network.h>
#include <sossrv.h>

/**
 * @file dev_network.c
 * @brief Open a TCP connection to the sensor network
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

int open_network_device(char* nicip)
{
  char *nicipaddr;
  char *nicportnum;
  int nicfd;
  struct sockaddr_in server_address;
  
  nicipaddr = strsep(&nicip,":");
  nicportnum = nicip;
  if (nicip == NULL){
    printf("Invalid port number\n");
    exit(EXIT_FAILURE);
    return -1;
  }
  // Create a socket
  if ((nicfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("sos_nic connect: socket");
      exit(EXIT_FAILURE);
    return -1;
  }

  // Setup the internet address
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(atoi(nicportnum));
  inet_aton(nicipaddr, &(server_address.sin_addr));
  memset(&(server_address.sin_zero), '\0', 8);

  // Connect
  if (connect(nicfd, (struct sockaddr *)&server_address, sizeof(struct sockaddr)) == -1) {
    perror("sos_nic connect: connect");
      exit(EXIT_FAILURE);
    return -1;
  }

  printf("Connected to SOS NIC @ IP address %s\n", inet_ntoa(server_address.sin_addr));
  printf("Connected to SOS_NIC @ port num %d\n", ntohs(server_address.sin_port));

  return nicfd;
}
