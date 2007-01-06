/*This ducument will have the implementation of the functions that will be
  defined for the matrix library*/
#include <math.h>
#include <matrix.h>
//#include <malloc_extmem.h>
/*
module matrixArithmeticsM
{
    provides interface matrixArithmetics;
    uses interface Leds;
}
*/
#define MAX8BIT 0xff		//maximum number that an 8-bit unsigned integer can hold
#define MAX16BIT 0xffff
int8_t
add (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B, CYCLOPS_Matrix * C)
{
  uint16_t i;			//used uint32_t because the Matrix size can be as large as (2^16-1)X(2^16-1)
  uint16_t size = A->rows * A->cols;

  //only perform operations if all three matrices of same size
  if ((A->rows != B->rows) || (B->rows != C->rows) || (A->cols != B->cols)
      || (B->cols != C->cols))
    return -EINVAL;
  else
    {
      uint8_t *A_ptr8;
      uint8_t *B_ptr8;
      uint8_t *C_ptr8;
      uint16_t *A_ptr16;
      uint16_t *B_ptr16;
      uint16_t *C_ptr16;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      B_ptr8 = ker_get_handle_ptr (B->data.hdl8);
      C_ptr8 = ker_get_handle_ptr (C->data.hdl8);
      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
      B_ptr16 = ker_get_handle_ptr (B->data.hdl16);
      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
      if ((A_ptr8 == NULL) || (B_ptr8 == NULL) || (C_ptr8 == NULL)
	  || (A_ptr16 == NULL) || (B_ptr16 == NULL) || (C_ptr16 == NULL))
	{
	  return -EINVAL;
	}
      if (A->depth == CYCLOPS_1BYTE && B->depth == CYCLOPS_1BYTE
	  && C->depth == CYCLOPS_1BYTE)

	{			//adding two matrices with elements uint8_t, and result also uint8_t
	  uint8_t sum;
	  for (i = 0; i < size; i++)

	    {
	      uint8_t dataA = A_ptr8[i], dataB = B_ptr8[i];
	      sum = dataA + dataB;

	      //sum = A_ptr8[i]+B_ptr8[i];
	      if (sum < dataA || sum < dataB)	//overflow occured
		C_ptr8[i] = MAX8BIT;

	      else
		C_ptr8[i] = sum;
	    }
	  return SOS_OK;
	}

      else if (A->depth == CYCLOPS_1BYTE && B->depth == CYCLOPS_1BYTE
	       && C->depth == CYCLOPS_2BYTE)

	{
	  for (i = 0; i < size; i++)	//here no overflow can occur
	    {
	      C_ptr16[i] = A_ptr8[i] + B_ptr8[i];
	    }
	  return SOS_OK;
	}

      else if (A->depth == CYCLOPS_2BYTE && B->depth == CYCLOPS_2BYTE
	       && C->depth == CYCLOPS_2BYTE)

	{
	  uint16_t sum;
	  for (i = 0; i < size; i++)

	    {
	      sum = A_ptr16[i] + B_ptr16[i];
	      if (sum < A_ptr16[i] || sum < B_ptr16[i])	//overflow occured
		C_ptr16[i] = MAX16BIT;

	      else
		C_ptr16[i] = sum;
	    }
	  return SOS_OK;
	}

      else
	return -EINVAL;
    }
}

int8_t
Sub (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B, CYCLOPS_Matrix * C)
{
  uint16_t i;			//used uint16_t, so max matrix size is 2^16
  uint16_t size = A->rows * A->cols;

  //only perform operations if all three matrices of same size
  if ((A->rows != B->rows) || (B->rows != C->rows) || (A->cols != B->cols)
      || (B->cols != C->cols))
    return -EINVAL;

  else

    {
      uint8_t *A_ptr8;
      uint8_t *B_ptr8;
      uint8_t *C_ptr8;
      uint16_t *A_ptr16;
      uint16_t *B_ptr16;
      uint16_t *C_ptr16;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      B_ptr8 = ker_get_handle_ptr (B->data.hdl8);
      C_ptr8 = ker_get_handle_ptr (C->data.hdl8);
      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
      B_ptr16 = ker_get_handle_ptr (B->data.hdl16);
      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
      if ((A_ptr8 == NULL) || (B_ptr8 == NULL) || (C_ptr8 == NULL)
	  || (A_ptr16 == NULL) || (B_ptr16 == NULL) || (C_ptr16 == NULL))
	{
	  return -EINVAL;
	}
      if (A->depth == CYCLOPS_1BYTE && B->depth == CYCLOPS_1BYTE
	  && C->depth == CYCLOPS_1BYTE)

	{			//adding two matrices with elements uint8_t, and result also uint8_t                    
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr8[i] < B_ptr8[i])	//overflow occured
		C_ptr8[i] = 0;

	      else
		C_ptr8[i] = A_ptr8[i] - B_ptr8[i];
	    }
	  return SOS_OK;
	}

      //changes made 12/22/04 checks if A has depth 2BYTE and C 1BYTE,
      //data.ptr_s changed accordingly
      else if (A->depth == CYCLOPS_2BYTE && B->depth == CYCLOPS_1BYTE
	       && C->depth == CYCLOPS_1BYTE)

	{
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr16[i] < B_ptr8[i])
		C_ptr8[i] = 0;

	      else
		C_ptr8[i] = A_ptr16[i] - B_ptr8[i];
	    }
	  return SOS_OK;
	}

      else if (A->depth == CYCLOPS_2BYTE && B->depth == CYCLOPS_2BYTE
	       && C->depth == CYCLOPS_2BYTE)

	{
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr16[i] < B_ptr16[i])	//overflow occured
		C_ptr16[i] = 0;

	      else
		C_ptr16[i] = A_ptr16[i] - B_ptr16[i];
	    }
	  return SOS_OK;
	}

      else
	return -EINVAL;
    }
}

int8_t
abssub (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B,
	CYCLOPS_Matrix * C)
{
  uint16_t i;			//used uint16_t, so max matrix size is 2^16
  uint16_t size = A->rows * A->cols;

  //only perform operations if all three matrices of same size
  if ((A->rows != B->rows) || (B->rows != C->rows) || (A->cols != B->cols)
      || (B->cols != C->cols))
    return -EINVAL;

  else

    {
      uint8_t *A_ptr8;
      uint8_t *B_ptr8;
      uint8_t *C_ptr8;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      B_ptr8 = ker_get_handle_ptr (B->data.hdl8);
      C_ptr8 = ker_get_handle_ptr (C->data.hdl8);
      if ((A_ptr8 == NULL) || (B_ptr8 == NULL) || (C_ptr8 == NULL))
	{
	  return -EINVAL;
	}
      if (A->depth == CYCLOPS_1BYTE && B->depth == CYCLOPS_1BYTE
	  && C->depth == CYCLOPS_1BYTE)

	{			//adding two matrices with elements uint8_t, and result also uint8_t                    
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr8[i] < B_ptr8[i])
		C_ptr8[i] = B_ptr8[i] - A_ptr8[i];

	      else
		C_ptr8[i] = A_ptr8[i] - B_ptr8[i];
	    }
	  return SOS_OK;
	}

      else
	return -EINVAL;
    }
}

int8_t
scale (const CYCLOPS_Matrix * A, CYCLOPS_Matrix * C, const uint32_t s)
{
  uint16_t i;			//used to traverse the arrays
  uint16_t size = (A->rows) * (A->cols);
  if (A->rows != C->rows || A->cols != C->cols)
    return -EINVAL;

  else

    {
      uint8_t *A_ptr8;
      uint8_t *C_ptr8;
      uint16_t *A_ptr16;
      uint16_t *C_ptr16;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      C_ptr8 = ker_get_handle_ptr (C->data.hdl8);
      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
      if ((A_ptr8 == NULL) || (C_ptr8 == NULL) || (A_ptr16 == NULL)
	  || (C_ptr16 == NULL))
	{
	  return -EINVAL;
	}
      if (A->depth == CYCLOPS_1BYTE && C->depth == CYCLOPS_1BYTE)

	{
	  uint8_t mul;
	  uint8_t max;
	  if (s > MAX8BIT)
	    mul = MAX8BIT;

	  else
	    mul = (uint8_t) s;
	  max = MAX8BIT / mul;	//maximum quantity that an element can be in order not to overflow the result
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr8[i] > max)	//overflow
		C_ptr8[i] = MAX8BIT;

	      else
		C_ptr8[i] = mul * (A_ptr8[i]);
	    }
	}

      else if (A->depth == CYCLOPS_1BYTE && C->depth == CYCLOPS_2BYTE)

	{			//scale a matrix with depth uint8_t, can only multiply by a number up to 256, and the resulting
	  //matrix has depth uint16_t therefore no overflow can occur
	  uint16_t mul;
	  uint16_t max;
	  if (s > MAX16BIT)
	    mul = MAX16BIT;

	  else
	    mul = s & MAX16BIT;
	  max = MAX16BIT / mul;
	  for (i = 0; i < size; i++)
	    if ((uint16_t) A_ptr8[i] > max)	//overflow
	      C_ptr16[i] = MAX16BIT;

	    else
	      C_ptr16[i] = mul * (uint16_t) (A_ptr8[i]);
	}

      else if (A->depth == CYCLOPS_2BYTE && C->depth == CYCLOPS_2BYTE)

	{
	  uint16_t mul;
	  uint16_t max;
	  if (s > MAX16BIT)
	    mul = MAX16BIT;

	  else
	    mul = s & MAX16BIT;
	  max = MAX16BIT / mul;	//maximum quantity that an element can be in order not to overflow the result
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr16[i] > max)	//overflow
		C_ptr16[i] = MAX16BIT;

	      else
		C_ptr16[i] = mul * (A_ptr16[i]);
	    }
	}

      else
	return -EINVAL;
      return SOS_OK;
    }
}

int8_t
getRow (const CYCLOPS_Matrix * A, CYCLOPS_Matrix * res, uint16_t row)
{
  uint16_t i;
  uint16_t offset;
  if (row > A->rows || row == 0)
    return -EINVAL;

  else

    {
      uint8_t *A_ptr8;
      uint8_t *res_ptr8;
      uint16_t *res_ptr16;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      res_ptr8 = ker_get_handle_ptr (res->data.hdl8);
      res_ptr16 = ker_get_handle_ptr (res->data.hdl16);
      if ((A_ptr8 == NULL) || (res_ptr8 == NULL) || (res_ptr16 == NULL))
	{
	  return -EINVAL;
	}
      res->rows = 1;
      res->cols = A->cols;
      offset = (row - 1) * A->cols;
      if (A->depth == CYCLOPS_1BYTE)
	{
	  for (i = 0; i < A->cols; i++)
	    res_ptr8[i] = A_ptr8[offset++];
	  return SOS_OK;
	}

      else if (A->depth == CYCLOPS_2BYTE)

	{
	  for (i = 0; i < A->cols; i++)
	    res_ptr16[i] = A_ptr8[offset++];
	  return SOS_OK;
	}

      else

	{
	  return -EINVAL;
	}
    }
}

int8_t
getCol (const CYCLOPS_Matrix * A, CYCLOPS_Matrix * res, uint16_t col)
{
  uint16_t i;
  uint16_t offset;
  if (col > A->cols || col == 0)
    return -EINVAL;

  else

    {
      uint8_t *A_ptr8;
      uint16_t *A_ptr16;
      uint8_t *res_ptr8;
      uint16_t *res_ptr16;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
      res_ptr8 = ker_get_handle_ptr (res->data.hdl8);
      res_ptr16 = ker_get_handle_ptr (res->data.hdl16);
      if ((A_ptr8 == NULL) || (A_ptr16 == NULL) || (res_ptr8 == NULL)
	  || (res_ptr16 == NULL))
	{
	  return -EINVAL;
	}
      res->rows = A->rows;
      res->cols = 1;
      offset = col - 1;
      if (A->depth == CYCLOPS_1BYTE)

	{
	  for (i = 0; i < A->rows; i++)

	    {
	      res_ptr8[i] = A_ptr8[offset];
	      offset += A->cols;
	    }
	}

      else if (A->depth == CYCLOPS_2BYTE)

	{
	  for (i = 0; i < A->rows; i++)

	    {
	      res_ptr16[i] = A_ptr16[offset];
	      offset += A->cols;
	    }
	}

      else
	return -EINVAL;
      return SOS_OK;
    }
}

int8_t
getBit (const CYCLOPS_Matrix * A, uint16_t row, uint16_t col)
{
  if (A->depth == CYCLOPS_1BIT && row < A->rows && col < A->cols && col != 0
      && row != 0)

    {
      uint8_t *A_ptr8;
      A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
      if ((A_ptr8 == NULL))
	{
	  return -EINVAL;
	}
      uint32_t location = (row - 1) * A->cols + (col - 1);
      uint8_t res = (A_ptr8[location / 8]) >> (7 - location % 8);
      res &= 0x01;
      return SOS_OK;
    }

  else
    return -EINVAL;
}

int8_t
threshold (const CYCLOPS_Matrix * A, CYCLOPS_Matrix * B, uint32_t t)
{
  uint32_t size = A->rows * A->cols;
  uint32_t i = 0;
  uint32_t sizeB = B->rows * B->cols;	//size of matrix B

  uint8_t *A_ptr8;
  uint8_t *B_ptr8;
  uint16_t *A_ptr16;
  A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
  B_ptr8 = ker_get_handle_ptr (B->data.hdl8);
  A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
  if ((A_ptr8 == NULL) || (B_ptr8 == NULL) || (A_ptr16 == NULL))
    {
      return -EINVAL;
    }
  for (i = 0; i < sizeB; i++)	//initialize B to zero
    B_ptr8[i] = 0;
  if (sizeB >= ceil (size / 8))

    {
      B->rows = A->rows;
      B->cols = A->cols;
      B->depth = CYCLOPS_1BIT;
      if (A->depth == CYCLOPS_1BYTE)

	{
	  uint8_t thresh;
	  if (t > MAX8BIT)
	    thresh = MAX8BIT;

	  else
	    thresh = (uint8_t) t;
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr8[i] >= thresh)

		{
		  B_ptr8[i / 8] |= (0x80 >> (i % 8));
		}
	    }
	  return SOS_OK;
	}

      else if (A->depth == CYCLOPS_2BYTE)

	{
	  uint16_t thresh;
	  if (t > MAX16BIT)
	    thresh = MAX16BIT;

	  else
	    thresh = (uint16_t) t;
	  for (i = 0; i < size; i++)

	    {
	      if (A_ptr16[i] >= thresh)
		B_ptr8[i / 8] |= (0x80 >> (i % 8));
	    }
	  return SOS_OK;
	}

      else
	return -EINVAL;
    }

  else
    return -EINVAL;
}
