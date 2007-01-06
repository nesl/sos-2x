#ifndef _ADCM1700_CONTROL_H_
#define _ADCM1700_CONTROL_H_
#include "image.h"

//*******************camera Voltage Regulator Functionality***************
// PORTD:2 is used for debugging (monitor camera Vcc)
#define MAKE_CAMERA_POWER_PIN_OUTPUT()	SET_CAMERA_EN_DD_OUT(); SET_TRIG_EN_DD_OUT()  // sbi(DDRB, 5);  sbi(DDRD, 2)
#define TURN_CAMERA_POWER_ON() CLR_CAMERA_EN(); SET_TRIG_EN()			      // cbi(PORTB, 5); sbi(PORTD,2)  //by making the pin low we turn on the P-Channel Mosfet
#define TURN_CAMERA_POWER_OFF() SET_CAMERA_EN(); CLR_TRIG_EN()			      // sbi(PORTB, 5); cbi(PORTD,2)  //by making the pin high we turn off the P-Channel Mosfet

//****************** Camera Module clock control *************************
// Cyclops2 with new CPLD firmware (direct uP control of MCLK through HS3)
#define MAKE_CAMERA_CLOCK_PIN_OUTPUT() SET_HS3_MCLK_EN_DD_OUT()			// sbi(DDRE, 4)
//#define GET_MCLK_STATE()  ((inp(PINE) & 0x10) >> 4)
#define TURN_CAMERA_CLOCK_ON() SET_HS3_MCLK_EN() 						// sbi(PORTE, 4)                   
#define TURN_CAMERA_CLOCK_OFF() CLR_HS3_MCLK_EN()						// cbi(PORTE, 4)

//--------------------------------------------------------
// INTERNAL KERNEL FUNCTIONS
//--------------------------------------------------------
extern uint8_t adcm1700control_init();

// Get the average pixel values
extern uint8_t getPixelAverages();

/**
 * \brief Data structure of IMAGER_READY message
 */
typedef struct {
  uint8_t status; //!< Imager Status
  uint16_t pad;  
} PACK_STRUCT msg_imager_t;


#ifndef _MODULE_

//--------------------------------------------------------
// CYCLOPS DRIVER API
//--------------------------------------------------------

// Mirror the MsgParam structure with more descriptive field names


/* 
 * @brief Registers the applicaiton module with the cyclops drivers and initializes the hardware
 * @param sos_pid_t pid application module id 
 * @return SOS_OK if succesful 
 * @return -EBUSY if unsucessful
 * @note Split-phase call, driver signals status through IMAGER_READY message.  
 */
extern int8_t ker_registerImagerClient(sos_pid_t pid);


/* 
 * @brief Release the imager resource held by the module
 * @param sos_pid_t pid application module id 
 * @return SOS_OK if succesful 
 * @return -EINVAL if unsucessful
 */
extern int8_t ker_releaseImagerClient(sos_pid_t pid);


/*
 * @brief captures an image and stores it in the external RAM
 * @param CYCLOPS_ImagePtr ptrImg Data structure which contains the handle to the image, number of frames to capture, size etc.
 * @return SOS_OK if succesful
 * @return -EINVAL if unsuccesful
 * @note Split-phase call, driver signals status through SNAP_IMAGE_DONE message.
 */
extern int8_t ker_snapImage(CYCLOPS_ImagePtr ptrImg);

// set new capture parameters
// (These values are not communicated to the module until the next capture is performed)
/*
 * @brief sets the parameters for the image to capture
 * @param CYCLOPS_CaputrePtr pCap 	data structure which contains the information about the image such as exposure, image size, white balance etc.
 * @return SOS_OK always
 */
extern int8_t ker_setCaptureParameters(CYCLOPS_CapturePtr pCap);

#endif


#endif// _ADCM1700_CONTROL_H_
