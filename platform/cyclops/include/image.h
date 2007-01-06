#ifndef IMAGE_H
#define IMAGE_H

#include <memConst.h>       /* for buffer pointer structure */

/* The ADCM-1700 sensor array size is 352 x 288 pixels. The capture parameters
 * may be used select the entire array, or a sub-region. However there are
 * several restrictions:
 * 1) The sizer is deactivated in RAW mode. Consequently the output size
 *    will equal the input size.
 * 2) The minimum output size is 24 x 24 pixels.
 * 3) The granualarity of the input window specification is 4 pixels. 
 */

typedef struct wpos
{
    int x;
    int y;
} wpos_t;

typedef struct wsize
{
    uint16_t x;
    uint16_t y;
} wsize_t;

typedef struct color8
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} color8_t;

typedef struct color16
{
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} color16_t;

typedef struct CYCLOPS_Capture_Parameters
{
  wpos_t    offset;             // Offset from center (nominally [0,0])
  wsize_t   inputSize;          // Input window (<= [352,288])
  uint8_t   testMode;           // normal or test mode capture
  float    exposurePeriod;      // used by AE procedure, (0 = auto -or-  exposure in seconds)
  color8_t analogGain;          // used by AE procedure, (0 = auto -or- analog gain: (1 + b[6]) * (1 + 0.15 * b[5:0]/(1 + 2 * b[7])) )
  color16_t digitalGain;        // used by AWB procedure,(0 = auto -or- digital gain * 128)
  uint16_t  runTime;            // equilibration time before capture
}CYCLOPS_Capture_Parameters;

typedef struct CYCLOPS_Capture_Parameters * CYCLOPS_CapturePtr;

typedef struct CYCLOPS_Image
{
  uint8_t  type;              // monochrome, RGB...
  wsize_t  size;              // image width in pixels
  uint8_t nFrames;            // number of sequential frames
  int16_t imageDataHandle;    // Ram - We will use handles to external memory
  //uint8_t *imageData;          // pointer to image data , later will change to handlers that is pointer to pointer when direct mem access is complete
  int8_t result;			// Result of image capture: filled in by imager driver
  bufferPtr imageHandle;       // image handler
}CYCLOPS_Image;

typedef  CYCLOPS_Image * CYCLOPS_ImagePtr;

// *** image types: ***
enum
    {
        CYCLOPS_IMAGE_TYPE_UNSET  =0,       // to force initialization ...
        CYCLOPS_IMAGE_TYPE_Y     =0x10,     // Gray Scale Intensity Image (1 byte per pixel)
        CYCLOPS_IMAGE_TYPE_RGB   =0x11,     // RGB in the eight bit format (3 bytes per pixel)
        CYCLOPS_IMAGE_TYPE_YCbCr =0x12,     // YCbCr fomat (2 bytes per pixel)
// There be dragons!!! The ADCM-1700 image processing pipeline (including the sizer) is bypassed 
// in RAW mode. The output size will equal
// the input size, regardless of the window settings!!!
	    CYCLOPS_IMAGE_TYPE_RAW   =0x13      // RAW format (1 byte per pixel)
    };

// *** capture test modes: ***
enum
    {
        CYCLOPS_TEST_MODE_UNSET,   // force initialization ...
        CYCLOPS_TEST_MODE_NONE,    // normal image capture
        CYCLOPS_TEST_MODE_SOC,     // sum of coordinates test mode
        CYCLOPS_TEST_MODE_8PB,     // 8-pixel wide border test mode
        CYCLOPS_TEST_MODE_CKB,     // checkerboard test mode
        CYCLOPS_TEST_MODE_BAR      // color bar test mode
    };


// *** module control ***
enum
    {
        CYCLOPS_STOP,
        CYCLOPS_RUN
    };

//enum
//    {
//        CYCLOPS_DEPTH_1U,
//        CYCLOPS_DEPTH_8U, 
//        CYCLOPS_DEPTH_8S//set_CPLD_run_mode(CPLD_OPCODE_RUN_CAMERA,0x11);
//    };
//


static inline uint8_t calcBytesPerPixel(uint8_t imgType)
{
  uint8_t bytesPerPixel;
  switch (imgType){
  case CYCLOPS_IMAGE_TYPE_Y:
    bytesPerPixel = 1;
    break;
  case CYCLOPS_IMAGE_TYPE_RGB:        
    bytesPerPixel = 3;
    break;
  case CYCLOPS_IMAGE_TYPE_YCbCr:
    bytesPerPixel = 2;
    break;
  default:
    bytesPerPixel = 0;
    break;
  }
  return bytesPerPixel;
}

static inline uint16_t imageSize(CYCLOPS_Image* pImg)
{
  uint16_t imgSize;
  imgSize = calcBytesPerPixel(pImg->type) * pImg->size.x * pImg->size.y * pImg->nFrames;
  return imgSize;
}

#endif // IMAGE_H
