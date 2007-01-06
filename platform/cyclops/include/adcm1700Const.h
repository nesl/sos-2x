#ifndef ADCM1700CONST_H
#define ADCM1700CONST_H

#define BLOCK_SWITCH_CODE 0x7f
#define TWO_BYTE_REGISTER 0x11
#define ONE_BYTE_REGISTER 0x22
#include <sos_types.h>
#include "image.h"

//static color16_t values;
typedef struct CYCLOPS_Image_result
{
	CYCLOPS_Image *  ImgPtr; 
	int8_t result;
} CYCLOPS_Image_result_t;

//static color16_t values;
typedef struct color16_result
{
	color16_t values;
	uint8_t result;
} color16_result_t;

typedef struct register_result
{
 uint16_t reg;
 uint16_t data;
 uint8_t status;
} register_result_t;
 
typedef struct block_result
{
 uint16_t startReg;
 char *data;
 uint8_t length;
 uint8_t result;
} block_result_t;
 
struct register_s
{
    uint16_t registerName;
    uint8_t block;
    uint8_t offset;
    uint8_t lenght;
}__attribute__((packed));

#define BLOCK_SWITCH_CODE 0x7f
#define TWO_BYTE_REGISTER 0x11
#define ONE_BYTE_REGISTER 0x22


enum {
    ADCM1700_CONTROL_ADDRESS=0x17,
    ADCM1700_EXPOSURE_ADDRESS,
    ADCM1700_FORMAT_ADDRESS,
    ADCM1700_PATCH_ADDRESS,
    ADCM1700_SNAP_ADDRESS,
    ADCM1700_RUN_ADDRESS,
    ADCM1700_VIDEO_ADDRESS,
    ADCM1700_WINDOWSIZE_ADDRESS,
    ADCM1700_STATISITICS_ADDRESS,
    ADCM1700_PATTERN_ADDRESS
};




//definition of camera regster index.
enum {
ADCM_REG_ID=0,       //ID of the device
ADCM_REG_CONTROL,
ADCM_REG_STATUS,
ADCM_REG_SIZE,      
ADCM_REG_SENSOR_WID_V,
ADCM_REG_SENSOR_HGT_V,
ADCM_REG_OUTPUT_WID_V,
ADCM_REG_OUTPUT_HGT_V,
ADCM_REG_SENSOR_WID_S,
ADCM_REG_SENSOR_HGT_S,
ADCM_REG_OUTPUT_WID_S,
ADCM_REG_OUTPUT_HGT_S,
ADCM_REG_OUTPUT_FORMAT,
ADCM_REG_OUTPUT_CTRL_V,
ADCM_REG_PROC_CTRL_V,
ADCM_REG_SZR_IN_WID_V,
ADCM_REG_SZR_IN_HGT_V,
ADCM_REG_SZR_OUT_WID_V,
ADCM_REG_SZR_OUT_HGT_V,
ADCM_REG_SZR_IN_WID_S,
ADCM_REG_SZR_IN_HGT_S,
ADCM_REG_SZR_OUT_WID_S,
ADCM_REG_SZR_OUT_HGT_S,
ADCM_REG_OUTPUT_CTRL_S,
ADCM_REG_PROC_CTRL_S,
ADCM_REG_PADR1,
ADCM_REG_PADR2,
ADCM_REG_PADR3,
ADCM_REG_PADR4,
ADCM_REG_PADR5,
ADCM_REG_PADR6,
ADCM_REG_FWROW,
ADCM_REG_FWCOL,
ADCM_REG_LWROW,
ADCM_REG_LWCOL,
ADCM_REG_SENSOR_CTRL,
ADCM_REG_DATA_GEN,
ADCM_REG_AF_CTRL1,
ADCM_REG_AF_CTRL2,
ADCM_REG_AE_TARGET,
ADCM_REG_EREC_PGA,
ADCM_REG_EROC_PGA,
ADCM_REG_OREC_PGA,
ADCM_REG_OROC_PGA,
ADCM_REG_APS_COEF_GRN1,
ADCM_REG_APS_COEF_RED,
ADCM_REG_APS_COEF_BLUE,
ADCM_REG_APS_COEF_GRN2,
ADCM_REG_RPT_V,
ADCM_REG_ROWEXP_L,
ADCM_REG_ROWEXP_H,
ADCM_REG_SROWEXP,
ADCM_REG_STATUS_FLAGS,
ADCM_REG_SUM_GRN1,
ADCM_REG_SUM_RED,
ADCM_REG_SUM_BLUE,
ADCM_REG_SUM_GRN2,
ADCM_REG_CPP_V,
ADCM_REG_HBLANK_V,
ADCM_REG_VBLANK_V,
ADCM_REG_CLK_PER,
ADCM_REG_AE2_ETIME_MIN,
ADCM_REG_AE2_ETIME_MAX,
ADCM_REG_AE_GAIN_MAX,

//NOTE: This is the size of enum. It is no a register in the imager space!
//**IMPORTANT** this should be always at the end of enum.
IMAGER_NUMBER_OF_REGISTERS 
};

/* definitions within register data */
#define ADCM_ID_1700                    0x0059  /* what should return when reading ADCM_REG_ID */
#define ADCM_ID_MASK                    0xF8    /* lowest three bits are revision # */
#define ADCM_STATUS_CONFIG_MASK         0x0004  /* Bit 2 = CONFIG */
#define ADCM_OUTPUT_CTRL_FRVCLK_MASK    0x0100  /* Bit 8 of OUTPUT_CTRL reg */
#define ADCM_OUTPUT_CTRL_FR_OUT_MASK    0x1000  /* Bit 12 of OUTPUT_CTRL reg */
#define ADCM_OUTPUT_CTRL_JUST_MASK      0x8000  /* Bit 15 of OUTPUT_CTRL reg */

/* Although the ADCM-1700 is capable of supporting different formats in video and still
   mode, one format is applied to both modes in Cyclops */   
#define ADCM_OUTPUT_FORMAT_RGB          0x0000  /* RGB (3 bytes per pixel) format */
#define ADCM_OUTPUT_FORMAT_YCbCr        0x0008  /* YCbCr Y1,U12,Y2,V12 (2 bytes per pixel) format */
// There be dragons!!! The ADCM-1700 Sizer is bypassed in RAW mode. The output size will equal
// the input size, regardless of the window settings!!!
#define ADCM_OUTPUT_FORMAT_RAW          0x000E  /* raw format (1 byte per pixel) format */
#define ADCM_OUTPUT_FORMAT_Y            0x000D  /* luminance format (1 byte per pixel) */

// Valid values for test pattern generator
#define ADCM_TEST_MODE_NONE    0x0000
#define ADCM_TEST_MODE_SOC     0x0003
#define ADCM_TEST_MODE_8PB     0x0004
#define ADCM_TEST_MODE_CKB     0x0005
#define ADCM_TEST_MODE_BAR     0x0007
#define ADCM_SIZE_VSIZE_QQVGA           0x0004
#define ADCM_SIZE_SSIZE_QQVGA           0x0400

#define ADCM_CONTROL_RUN_MASK           0x0001
#define ADCM_CONTROL_SNAP_MASK          0x0002
#define ADCM_CONTROL_CONFIG_MASK        0x0004  /* bit 2 in control register */

#define ADCM_SIZE_VSIZE_MASK            0x0007
#define ADCM_SIZE_SSIZE_MASK            0x0700
#define ADCM_SIZE_VSIZE_QQVGA           0x0004
#define ADCM_SIZE_SSIZE_QQVGA           0x0400
#define ADCM_PROC_CTRL_NOSIZER          0x0010  /* bit 4 of PROC_CTRL */

// auto function control
#define ADCM_AF_AE  0x0001
#define ADCM_AF_AWB 0x0002
#define ADCM_AF_ABL 0x0010

//register_param_t 
extern struct register_s register_param_list[];
extern struct register_s *lookup_register(uint16_t registerName);

#define GET_REGISTER_BLOCK(myRegister) lookup_register(myRegister)->block
#define GET_REGISTER_OFFSET(myRegister) lookup_register(myRegister)->offset
#define GET_REGISTER_LENGHT(myRegister) lookup_register(myRegister)->lenght

// size limits for adcm1700:
#define ADCM_SIZE_MAX_X 352
#define ADCM_SIZE_MIN_X  24
#define ADCM_SIZE_MAX_Y 288
#define ADCM_SIZE_MIN_Y  24


#define SET_SIZE_USING_SIZER  // use sizer instead of low-level windowing
/* default image type and size information */
#define ADCM_SIZE_1700DEFAULT_W 352     /* columns */
#define ADCM_SIZE_1700DEFAULT_H 288     /* rows */
#define ADCM_SIZE_API_DEFAULT_W  64     /* columns */
#define ADCM_SIZE_API_DEFAULT_H  64     /* rows */
/* end default */
/* SIZE macro definitions (specific to to the 1700 imager */
#define FWROW_1700(row)  ( (((ADCM_SIZE_1700DEFAULT_H - (row))/2) + 4) /4 )
#define FWCOL_1700(col)  ( (((ADCM_SIZE_1700DEFAULT_W - (col))/2) + 4) /4 )
#define LWROW_1700(row)  ( (((ADCM_SIZE_1700DEFAULT_H - (row))/2) + (row) + 8) /4 )
#define LWCOL_1700(col)  ( (((ADCM_SIZE_1700DEFAULT_W - (col))/2) + (col) + 8) /4 )    

#endif

// 2 horizontal lines between frames (provides extra time to stop imager before next frame begins)
#define ADCM_VBLANK_DEFAULT  2
