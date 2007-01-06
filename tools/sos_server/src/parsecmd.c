/* -*-C-*- */
/** 
 * \file parsecmd.c
 * \brief The command line parser utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <parsecmd.h>
#include <sossrv.h>

//---------------------------------------------------------------------------------
// COMMAND LINE PARSER
int parsecmdline(int argc, char *argv[], int* pServerPort, char** pSerialPort, int* pBaudRate, char** networkPort, int *outputOpts)
{
  int ch;
  
  while((ch = getopt(argc, argv, "hqQdp:s:b:n:")) != -1) {
    switch(ch) {
    case 'p': (*pServerPort) = (int)atoi(optarg); break;
    case 's': *pSerialPort = optarg; break;
    case 'b': (*pBaudRate) = (int)atoi(optarg); break;
    case 'n': *networkPort = optarg; break;
    case 'Q': if ((*outputOpts <= OUTPUT_DEFAULT) && (*outputOpts > OUTPUT_SILENT)) { *outputOpts = OUTPUT_SILENT; } break;
    case 'q': if ((*outputOpts <= OUTPUT_DEFAULT) && (*outputOpts > OUTPUT_QUIET)) { *outputOpts = OUTPUT_QUIET; } break;
    case 'd': if (*outputOpts < OUTPUT_DEBUG) { *outputOpts = OUTPUT_DEBUG; } break;
    case '?': case 'h':
      printusage();
      break;    
    }
  }
  return 0;
}

//---------------------------------------------------------------------------------
// ERROR EXIT FUNCTION
int printusage()
{
  printf("Sossrv Command Line Usage:\n");
  printf("sossrv [-p <Port>] [-s <COM Port>] [-n <TCP Port>] [-b <baudrate>] [-h]\n");
  printf(" -q              Reduced output (one line headers)\n");
  printf(" -Q              Really quiet, output only on errors\n");
  printf(" -d              Debug, print raw uart streams\n");
  printf(" -s <COM Port>  SOS NIC COM Port. COM Port can be local device e.g. /dev/ttyUSB0\n");
  printf("                Default = %s\n", DEFAULT_SERIAL_PORT);
  printf(" -p <Port>      The port number of the sossrv server. Default = 7915\n");
  printf(" -n <TCP Port>  TCP Port can be <IP Addr:Port Num> e.g. 192.69.10.3:6009\n");
  printf(" -b <baudrate>  SOS NIC Baudrate.\n");
  printf("                Default = %d bps\n", DEFAULT_BAUDRATE);
  printf(" -h             Print this help message\n");
  exit(EXIT_FAILURE);
  return 0;
}
