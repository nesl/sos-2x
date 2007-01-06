#ifndef ADCM1700COMM_H
#define ADCM1700COMM_H

enum
    {
        IMAGER_COMM_TEST=0x11,
        IMAGER_COMM_FORMAT,
        IMAGER_COMM_WINDOW_SIZE,
        IMAGER_COMM_VIDEO,
        IMAGER_COMM_SNAP,
        IMAGER_COMM_EXPOSER
    };
	
extern int8_t adcm1700ImagerCommInit();
extern int8_t writeRegister(sos_pid_t id, uint16_t reg, uint16_t data);
extern int8_t readRegister(sos_pid_t id, uint16_t reg);
extern int8_t writeBlock(sos_pid_t id, uint16_t startReg, char *data,uint8_t length);
extern int8_t readBlock(sos_pid_t id, uint16_t startReg,char *data,uint8_t length);

#endif
