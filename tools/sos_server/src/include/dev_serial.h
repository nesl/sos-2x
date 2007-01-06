/* -*-C-*- */

/**
 * \file dev_serial.h
 * \brief The API for the serial driver on Linux
 */

#ifdef _DEV_SERIAL_H_
#define _DEV_SERIAL_H_
 
/**
 * \fn int open_serial_device(char* dev, int speed, int* fd)
 * \brief Opens a serial port and returns its file descriptor through a pointer
 * \param char* dev  The name of the serial device. For e.g. \dev\ttyS0
 * \param int speed  The speed of the serial port
 * \param int* fd    The pointer to hold the file descriptor of the serial device
 * \return 0 if success
 */
int open_serial_device(char* dev, int speed, int* fd);

/**
 * \fn int close_serial_device()
 * \brief Close the serial device
 */
int close_serial_device();


#endif // _DEV_SERIAL_H_
