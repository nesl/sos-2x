/* -*-C-*- */
/**
 * \file dev_serial.c
 * \brief The serial device driver for Linux (Adopted from Serial-Programming HOWTO)
 */


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <termios.h>
#include <dev_serial.h>
#include <sossrv.h>

int getspeedconstant(int speed); // The mapping from speed to the constant identifier
int dev_serial_fd;               // File descriptor of the serial device
struct termios oldtio,newtio;    // Termios Structure for configuring the serial data

int open_serial_device(char* dev, int speed, int* fd)
{
  int realspeed;
  realspeed = getspeedconstant(speed);
  dev_serial_fd = open(dev, O_RDWR | O_NOCTTY);                    // Open serial device 
  if (dev_serial_fd <0) {
    perror("open_serial_device: open"); 
    exit(EXIT_FAILURE);
  }
  DEBUG("Serial file descriptor: %d\n", dev_serial_fd);
  DEBUG("Attempting to set serial port parameters\n");
  tcgetattr(dev_serial_fd,&oldtio);                                 // Save current port settings
  bzero(&newtio, sizeof(newtio));                                   // Initialize the new termios structure
  newtio.c_cflag = CS8 | CLOCAL | CREAD | HUPCL;                    // Control mode flags - 8 N 1
  newtio.c_iflag = IGNBRK | IGNPAR;                                 // Input mode flags - Ignore break condition, Ignore parity errors
  newtio.c_oflag = 0;                                               // Output mode flags
  newtio.c_lflag = 0;                                               // Local mode flags - Non Canonical, No Echo
  newtio.c_cc[VTIME]    = 0;                                        // No inter-character timer unused
  newtio.c_cc[VMIN]     = 1;                                        // Blocking read until 1 char is received
  cfsetispeed(&newtio, realspeed);                                  // Baudrate setting 
  cfsetospeed(&newtio, realspeed);                                  // Baudrate setting
  tcflush(dev_serial_fd, TCIFLUSH);                                 // Flush the serial device
  tcsetattr(dev_serial_fd,TCSANOW,&newtio);                         // Set the new serial attributes
  *(fd) = dev_serial_fd;                                            // Copy the file descriptor to the application
  printf("Connected to SOS_NIC @ serial port: %s\n", dev);
  printf("Connected to SOS_NIC @ baud rate: %d\n", speed);
  return 0;
}



int close_serial_device()
{
  tcsetattr(dev_serial_fd,TCSANOW,&oldtio);
}


int getspeedconstant(int speed)
{
	switch (speed){
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
		default:
			printf("Unsupported baudrate. Exiting ...\n");
			exit(EXIT_FAILURE);
	}
	return 0;
}
