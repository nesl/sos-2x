/*
    File:		SerialPortSample.c
	
    Description:	This sample demonstrates how to use IOKitLib to find all serial ports on the system.
                        It also shows how to open, write to, read from, and close a serial port.
                
    Copyright:		© Copyright 2000-2002 Apple Computer, Inc. All rights reserved.
	
    Disclaimer:		IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
                        ("Apple") in consideration of your agreement to the following terms, and your
                        use, installation, modification or redistribution of this Apple software
                        constitutes acceptance of these terms.  If you do not agree with these terms,
                        please do not use, install, modify or redistribute this Apple software.

                        In consideration of your agreement to abide by the following terms, and subject
                        to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
                        copyrights in this original Apple software (the "Apple Software"), to use,
                        reproduce, modify and redistribute the Apple Software, with or without
                        modifications, in source and/or binary forms; provided that if you redistribute
                        the Apple Software in its entirety and without modifications, you must retain
                        this notice and the following text and disclaimers in all such redistributions of
                        the Apple Software.  Neither the name, trademarks, service marks or logos of
                        Apple Computer, Inc. may be used to endorse or promote products derived from the
                        Apple Software without specific prior written permission from Apple.  Except as
                        expressly stated in this notice, no other rights or licenses, express or implied,
                        are granted by Apple herein, including but not limited to any patent rights that
                        may be infringed by your derivative works or by other works in which the Apple
                        Software may be incorporated.

                        The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
                        WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
                        WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
                        PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
                        COMBINATION WITH YOUR PRODUCTS.

                        IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
                        CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
                        GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
                        ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
                        OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
                        (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
                        ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
				
	Change History (most recent first):
        
            <4>		10/15/02	Add CodeWarrior project (r. 2797719).
					Consider local echo in modem response parsing (r. 2985626).
					Add examples of additional POSIX serial calls and man page references.
	    <3>		12/19/00	New IOKit keys.
            <2>	 	08/22/00	Incorporated changes from code review.
            <1>	 	08/03/00	New sample.
        
*/
/**
 * @file dev_serial_mac.c
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @brief Serial IO for MAC OS X
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#ifdef __MWERKS__
#define __CF_USE_FRAMEWORK_INCLUDES__
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>

#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dev_serial.h>
#include <sossrv.h>

// Function prototypes
static kern_return_t FindModems(io_iterator_t *matchingServices);
static kern_return_t GetModemPath(io_iterator_t serialPortIterator, char *bsdPath, CFIndex maxPathSize, char* dev);



int getspeedconstant(int speed); //! The mapping from speed to the constant identifier
int dev_serial_fd;               //! File descriptor of the serial device
struct termios oldtio,newtio;    //! Termios Structure for configuring the serial data

int open_serial_device(char* dev, int speed, int* fd)
{
  int realspeed;
  int fileDescriptor;
  kern_return_t	kernResult; // on PowerPC this is an int (4 bytes)
  /*
   *	error number layout as follows (see mach/error.h):
   *
   *	hi		 		       lo
   *	| system(6) | subsystem(12) | code(14) |
   */
  
  io_iterator_t	serialPortIterator;
  char		bsdPath[MAXPATHLEN];
  
  realspeed = getspeedconstant(speed);
  kernResult = FindModems(&serialPortIterator);
  kernResult = GetModemPath(serialPortIterator, bsdPath, sizeof(bsdPath), dev);
  IOObjectRelease(serialPortIterator);	// Release the iterator.
  
  // Now open the modem port we found, initialize the modem, then close it
  if (!bsdPath[0]){
    printf("No modem port found.\n");
    exit(EXIT_FAILURE);
  }

  dev_serial_fd = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);    //! Open serial device in a non-blocking mode
  if (dev_serial_fd <0) {
	perror("open_serial_device: open"); 
	exit(EXIT_FAILURE);
  }

  if (ioctl(dev_serial_fd, TIOCEXCL) == -1){
    printf("Error setting TIOCEXCL on %s - %s(%d).\n",
	   bsdPath, strerror(errno), errno);
    exit(EXIT_FAILURE);
  }

  // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
  // See fcntl(2) ("man 2 fcntl") for details.
  
  if (fcntl(dev_serial_fd, F_SETFL, 0) == -1){
    printf("Error clearing O_NONBLOCK %s - %s(%d).\n",
	   bsdPath, strerror(errno), errno);
    exit(EXIT_FAILURE);
  }  


  tcgetattr(dev_serial_fd,&oldtio);                                 //! Save current port settings
  bzero(&newtio, sizeof(newtio));                                   //! Initialize the new termios structure
  newtio.c_cflag = CS8 | CLOCAL | CREAD | HUPCL;                    //! Control mode flags - 8 N 1
  newtio.c_iflag = IGNBRK | IGNPAR;                                 //! Input mode flags - Ignore break condition, Ignore parity errors
  newtio.c_oflag = 0;                                               //! Output mode flags
  newtio.c_lflag = 0;                                               //! Local mode flags - Non Canonical, No Echo
  newtio.c_cc[VTIME]    = 0;                                        //! No inter-character timer unused
  newtio.c_cc[VMIN]     = 1;                                        //! Blocking read until 1 char is received
  cfsetispeed(&newtio, realspeed);                                  //! Baudrate setting 
  cfsetospeed(&newtio, realspeed);                                  //! Baudrate setting
  tcflush(dev_serial_fd, TCIFLUSH);                                 //! Flush the serial device
  tcsetattr(dev_serial_fd,TCSANOW,&newtio);                         //! Set the new serial attributes
  *(fd) = dev_serial_fd;                                            //! Copy the file descriptor to the application
  DEBUG("Connected to SOS_NIC @ serial port: %s\n", dev);
  DEBUG("Connected to SOS_NIC @ baud rate: %d\n", speed);
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




// Returns an iterator across all known modems. Caller is responsible for
// releasing the iterator when iteration is complete.
static kern_return_t FindModems(io_iterator_t *matchingServices)
{
    kern_return_t		kernResult; 
    mach_port_t			masterPort;
    CFMutableDictionaryRef	classesToMatch;

/*! @function IOMasterPort
    @abstract Returns the mach port used to initiate communication with IOKit.
    @discussion Functions that don't specify an existing object require the IOKit master port to be passed. This function obtains that port.
    @param bootstrapPort Pass MACH_PORT_NULL for the default.
    @param masterPort The master port is returned.
    @result A kern_return_t error code. */

    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOMasterPort returned %d\n", kernResult);
	goto exit;
    }
        
/*! @function IOServiceMatching
    @abstract Create a matching dictionary that specifies an IOService class match.
    @discussion A very common matching criteria for IOService is based on its class. IOServiceMatching will create a matching dictionary that specifies any IOService of a class, or its subclasses. The class is specified by C-string name.
    @param name The class name, as a const C-string. Class matching is successful on IOService's of this class or any subclass.
    @result The matching dictionary created, is returned on success, or zero on failure. The dictionary is commonly passed to IOServiceGetMatchingServices or IOServiceAddNotification which will consume a reference, otherwise it should be released with CFRelease by the caller. */

    // Serial devices are instances of class IOSerialBSDClient
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else {
/*!
	@function CFDictionarySetValue
	Sets the value of the key in the dictionary.
	@param theDict The dictionary to which the value is to be set. If this
		parameter is not a valid mutable CFDictionary, the behavior is
		undefined. If the dictionary is a fixed-capacity dictionary and
		it is full before this operation, and the key does not exist in
		the dictionary, the behavior is undefined.
	@param key The key of the value to set into the dictionary. If a key 
		which matches this key is already present in the dictionary, only
		the value is changed ("add if absent, replace if present"). If
		no key matches the given key, the key-value pair is added to the
		dictionary. If added, the key is retained by the dictionary,
		using the retain callback provided
		when the dictionary was created. If the key is not of the sort
		expected by the key retain callback, the behavior is undefined.
	@param value The value to add to or replace into the dictionary. The value
		is retained by the dictionary using the retain callback provided
		when the dictionary was created, and the previous value if any is
		released. If the value is not of the sort expected by the
		retain or release callbacks, the behavior is undefined.
*/
        CFDictionarySetValue(classesToMatch,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDRS232Type));
        // Each serial device object has a property with key
        // kIOSerialBSDTypeKey and a value that is one of kIOSerialBSDAllTypes,
        // kIOSerialBSDModemType, or kIOSerialBSDRS232Type. You can experiment with the
        // matching by changing the last parameter in the above call to CFDictionarySetValue.
        
        // As shipped, this sample is only interested in modems,
        // so add this property to the CFDictionary we're matching on. 
        // This will find devices that advertise themselves as modems,
        // such as built-in and USB modems. However, this match won't find serial modems.
    }
    
    /*! @function IOServiceGetMatchingServices
        @abstract Look up registered IOService objects that match a matching dictionary.
        @discussion This is the preferred method of finding IOService objects currently registered by IOKit. IOServiceAddNotification can also supply this information and install a notification of new IOServices. The matching information used in the matching dictionary may vary depending on the class of service being looked up.
        @param masterPort The master port obtained from IOMasterPort().
        @param matching A CF dictionary containing matching information, of which one reference is consumed by this function. IOKitLib can contruct matching dictionaries for common criteria with helper functions such as IOServiceMatching, IOOpenFirmwarePathMatching.
        @param existing An iterator handle is returned on success, and should be released by the caller when the iteration is finished.
        @result A kern_return_t error code. */

    kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, matchingServices);    
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
	goto exit;
    }
        
exit:
    return kernResult;
}
    
// Given an iterator across a set of modems, return the BSD path to the first one.
// If no modems are found the path name is set to an empty string.
static kern_return_t GetModemPath(io_iterator_t serialPortIterator, char *bsdPath, CFIndex maxPathSize, char* dev)
{
    io_object_t		modemService;
    kern_return_t	kernResult = KERN_FAILURE;
    Boolean		modemFound = false;
    printf("Scanning IO ports ...\n");
    // Initialize the returned path
    *bsdPath = '\0';
    
    // Iterate across all modems found. In this example, we bail after finding the first modem.
    
    while ((modemService = IOIteratorNext(serialPortIterator)) && !modemFound)
    {
        CFTypeRef	bsdPathAsCFString;

	// Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
	// used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
	// incoming calls, e.g. a fax listener.
	
	bsdPathAsCFString = IORegistryEntryCreateCFProperty(modemService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0);
        if (bsdPathAsCFString)
        {
            Boolean result;
            
            // Convert the path from a CFString to a C (NUL-terminated) string for use
	    // with the POSIX open() call.
	    
	    result = CFStringGetCString(bsdPathAsCFString,
                                        bsdPath,
                                        maxPathSize, 
                                        kCFStringEncodingASCII);
            CFRelease(bsdPathAsCFString);
            
            if (result)
	    {
	      printf("BSD path: %s ...", bsdPath);
	      if (0 == strcmp(bsdPath, dev)){
                modemFound = true;
                kernResult = KERN_SUCCESS;
		printf("Match\n");
	      }
	      else{
		*bsdPath = '\0';
		printf("No Match\n");
	      }
	    }
        }

        printf("\n");

        // Release the io_service_t now that we are done with it.
	
	(void) IOObjectRelease(modemService);
    }
        
    return kernResult;
}
