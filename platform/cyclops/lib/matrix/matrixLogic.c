/*This ducument will have the implementation of the functions that will be
  defined for the matrix library*/
#include <matrix.h>
//#include <malloc_extmem.h>
/*
module matrixLogicM
{
    provides interface matrixLogic;
    uses interface Leds;
}
*/
#define MAX8BIT 0xff		//maximum number that an 8-bit unsigned integer can hold
#define MAX16BIT 0xffff
int8_t
and (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B, CYCLOPS_Matrix * C)
{
  if ((A->rows == B->rows) && (B->rows == C->rows) && (A->cols == B->cols)
      && (B->cols == C->cols))

    {
      uint16_t i = 0;
      uint16_t size = (A->cols) * (A->rows);
      if (A->depth == B->depth && B->depth == C->depth)

	{
	  if (A->depth == CYCLOPS_1BYTE || A->depth == CYCLOPS_1BIT)

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
	      for (i = 0; i < size; i++)
		{
		  C_ptr8[i] = (A_ptr8[i]) & (B_ptr8[i]);
		}
	      return SOS_OK;
	    }

	  else if (A->depth == CYCLOPS_2BYTE)

	    {

	      uint16_t *A_ptr16;
	      uint16_t *B_ptr16;
	      uint16_t *C_ptr16;
	      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
	      B_ptr16 = ker_get_handle_ptr (B->data.hdl16);
	      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
	      if ((A_ptr16 == NULL) || (B_ptr16 == NULL) || (C_ptr16 == NULL))
		{
		  return -EINVAL;
		}
	      for (i = 0; i < size; i++)
		{
		  C_ptr16[i] = (A_ptr16[i]) & (B_ptr16[i]);
		}
	      return SOS_OK;
	    }

	  else
	    return -EINVAL;
	}

      else
	return -EINVAL;
    }

  else
    return -EINVAL;
}

int8_t
or (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B, CYCLOPS_Matrix * C)
{
  if ((A->rows == B->rows) && (B->rows == C->rows) && (A->cols == B->cols)
      && (B->cols == C->cols))

    {
      uint16_t i = 0;
      uint16_t size = (A->cols) * (A->rows);
      if (A->depth == B->depth && B->depth == C->depth)

	{
	  if (A->depth == CYCLOPS_1BYTE || A->depth == CYCLOPS_1BIT)

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

	      for (i = 0; i < size; i++)
		{
		  C_ptr8[i] = (A_ptr8[i]) | (B_ptr8[i]);
		}
	      return SOS_OK;
	    }

	  else if (A->depth == CYCLOPS_2BYTE)

	    {
	      uint16_t *A_ptr16;
	      uint16_t *B_ptr16;
	      uint16_t *C_ptr16;
	      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
	      B_ptr16 = ker_get_handle_ptr (B->data.hdl16);
	      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
	      if ((A_ptr16 == NULL) || (B_ptr16 == NULL) || (C_ptr16 == NULL))
		{
		  return -EINVAL;
		}
	      for (i = 0; i < size; i++)
		{
		  C_ptr16[i] = (A_ptr16[i]) | (B_ptr16[i]);
		}
	      return SOS_OK;
	    }

	  else
	    return -EINVAL;
	}

      else
	return -EINVAL;
    }

  else
    return -EINVAL;
}

int8_t
xor (const CYCLOPS_Matrix * A, const CYCLOPS_Matrix * B, CYCLOPS_Matrix * C)
{
  if ((A->rows == B->rows) && (B->rows == C->rows) && (A->cols == B->cols)
       && (B->cols == C->cols))

    {
      uint16_t i = 0;
      uint16_t size = (A->cols) * (A->rows);
      if (A->depth == B->depth && B->depth == C->depth)

	{
	  if (A->depth == CYCLOPS_1BYTE || A->depth == CYCLOPS_1BIT)

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
	      for (i = 0; i < size; i++)
		{
		  C_ptr8[i] = (A_ptr8[i]) ^ (B_ptr8[i]);
		}
	      return SOS_OK;
	    }

	  else if (A->depth == CYCLOPS_2BYTE)

	    {
	      uint16_t *A_ptr16;
	      uint16_t *B_ptr16;
	      uint16_t *C_ptr16;
	      A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
	      B_ptr16 = ker_get_handle_ptr (B->data.hdl16);
	      C_ptr16 = ker_get_handle_ptr (C->data.hdl16);
	      if ((A_ptr16 == NULL) || (B_ptr16 == NULL) || (C_ptr16 == NULL))
		{
		  return -EINVAL;
		}
	      for (i = 0; i < size; i++)
		{
		  C_ptr16[i] = (A_ptr16[i]) ^ (B_ptr16[i]);
		}
	      return SOS_OK;
	    }

	  else
	    return -EINVAL;
	}

      else
	return -EINVAL;
    }

  else
    return -EINVAL;
}
