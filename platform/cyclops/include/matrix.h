/*
 *Authors: 
 * Obi Iroezi: obimdina@seas.ucla.edu
 * Juan Carlos Garcia: yahualic@ucla.edu
 * Mohammad Rahimi: mhr@cens.ucla.edu
 *
 *
 *This document will define the necessary macro definitions for the matrix
  library support for matrix operations*/
#ifndef MATRIX_H
#define MATRIX_H
#include <sos_types.h>

#include <image.h>
#include <malloc_extmem.h>


//****************************************************************************
// FUNCTION IDs
//****************************************************************************
enum {
	CONVERT_IMAGE_TO_MATRIX_FID, 
	CONVERT_RGB_TO_MATRIX_FID,
};

//****************************************************************************
//SHAPES
//****************************************************************************
//structures to represent a two dimensional point whose coordinates are 8 bit unsigned integers

typedef struct //cyclopsPoint2d8u
{
    uint8_t x; //x coordinate
    uint8_t y; //y coordinate
}CYCLOPS_Point2d8u;


//structure to represent a rectangle in a plane
typedef struct cyclopsRectangle
{
    uint8_t x;       //x offset, or position of lower left corner
    uint8_t y;       //y offset
    uint8_t width;   //width of rectangle
    uint8_t height;  //height of rectangle
}CYCLOPS_Rectangle;

//structure to represent a line in a plane
typedef struct cyclopsLine
{
    int m; //slope of the line
    int b; //y-intercept
}CYCLOPS_Line;

//****************************************************************************
//MATRIX
//****************************************************************************
//MATRIX-Depth
enum {CYCLOPS_1BIT=0,CYCLOPS_1BYTE=1,CYCLOPS_2BYTE=2,CYCLOPS_4BYTE=4};
//used when converting matrices to represent the color that will be extracted
enum {CYCLOPS_RED= 0,CYCLOPS_GREEN = 1,CYCLOPS_BLUE = 2};  

typedef struct RGB_to_matrix
{
	CYCLOPS_Image *  ImgPtr; 
	uint8_t color;
} RGB_to_matrix_t;

//structure representation of a matrix
typedef struct{
    uint8_t depth;    //stores how many bytes are in each elemnt, CYCLOPS_XBYTE where X is the num of bytes in each element
    union
    {
     //   uint8_t* ptr8;    //for matrices of uint8 elements
      //  uint16_t* ptr16;
       // uint32_t* ptr32;
	 int16_t hdl8;
	int16_t hdl16;
	int16_t hdl32;	    
    }data;
    uint16_t rows; //number of rows
    uint16_t cols; //number of columns*/
} CYCLOPS_Matrix;


//Kevin - is it better to combine all these structs?
typedef struct matrix_math
{
	CYCLOPS_Image *  ImgPtrA; 
	CYCLOPS_Image *  ImgPtrB; 
} matrix_math_t;

typedef struct matrix_location
{
	CYCLOPS_Matrix *  matPtr; 
	uint16_t row; 
	uint16_t col; 
} matrix_location_t;

typedef struct matrix_thresh
{
	CYCLOPS_Matrix *  matPtr; 
	uint32_t threshhold_value ; 
} matrix_thresh_t;

typedef struct matrix_scale
{
	CYCLOPS_Matrix *  matPtrA; 
	CYCLOPS_Matrix *  matPtrC; 
	uint32_t scale_value ; 
} matrix_scale_t;

#ifndef _MODULE_

extern int8_t add(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C);
extern int8_t Sub(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C);
extern int8_t abssub(const CYCLOPS_Matrix* A, const CYCLOPS_Matrix* B, CYCLOPS_Matrix* C);
extern int8_t scale(const CYCLOPS_Matrix* A,CYCLOPS_Matrix* C,const uint32_t s);
extern int8_t getRow(const CYCLOPS_Matrix* A,CYCLOPS_Matrix* res, uint16_t row);
extern int8_t getCol(const CYCLOPS_Matrix* A,CYCLOPS_Matrix* res, uint16_t col);
extern int8_t getBit(const CYCLOPS_Matrix* A, uint16_t row, uint16_t col);
extern int8_t threshold(const CYCLOPS_Matrix* A, CYCLOPS_Matrix* B, uint32_t t);
extern int8_t convertImageToMatrix(CYCLOPS_Matrix* M, const CYCLOPS_Image* Im);
extern int8_t convertRGBToMatrix(CYCLOPS_Matrix* M, const CYCLOPS_Image* Im, uint8_t color);

#endif
#endif
