/*This ducument will have the implementation of the functions that will be
  defined for the matrix library*/
  
#include <sos_types.h>
#include <matrix.h>
#include <basicStat.h>

#define MAX8BIT 0xff //maximum number that an 8-bit unsigned integer can hold
#define MAX16BIT 0xffff

int16_t min(CYCLOPS_Matrix* A)
{
    uint16_t min;
    uint16_t i=0;
    uint16_t size=(A->cols) * (A->rows);
    if(A->depth == CYCLOPS_1BYTE)
        {
            uint8_t possibleMin = MAX8BIT;
		
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}				
		
		for(i=0;i<size;i++)
		{
			if(A_ptr8[i]<possibleMin) 
			{
				possibleMin = A_ptr8[i];
			}
		}
            min = (uint16_t) possibleMin;
        }
   else if(A->depth==CYCLOPS_2BYTE)
        {
            uint16_t possibleMin = MAX16BIT;
		uint16_t *A_ptr16;
		A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
		if ( A_ptr16 == NULL) 
		{
			return -EINVAL;
		}				
		
            for(i=0;i<size;i++)
                if(A_ptr16[i]<possibleMin)
		{
			possibleMin = A_ptr16[i];
		}
            min = possibleMin;
        }
    else
        {
            return 0;
        }
    return min;
}

int16_t max(CYCLOPS_Matrix* A)
{
    uint16_t max;
    uint16_t i=0;
    uint16_t size=(A->cols) * (A->rows);
    if(A->depth==CYCLOPS_1BYTE)
        {
            uint8_t possibleMax=0;		
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}						
		for(i=0;i<size;i++)
		{
			if(A_ptr8[i]>possibleMax) 
			{
				possibleMax = A_ptr8[i];
			}
		}
            max = (uint16_t) possibleMax;
        }
    else if(A->depth==CYCLOPS_2BYTE)
        {
		uint16_t possibleMax=0;

		uint16_t *A_ptr16;
		A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
		if ( A_ptr16 == NULL) 
		{
			return -EINVAL;
		}		
		for(i=0;i<size;i++)
		{
			if(A_ptr16[i]>possibleMax) 
			{
				possibleMax = A_ptr16[i];
			}
		}
            max = possibleMax;
        }
    else
        {
            return -EINVAL;  //could also return zero if the max value is zero
        }
    return max;
}

int8_t maxLocate(CYCLOPS_Matrix* A, uint8_t* row, uint8_t* col)
{
    uint8_t possibleMax=0;
    uint8_t i=0;
    uint8_t j=0;
				
    if(A->depth==CYCLOPS_1BYTE)
        {
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}	
            for(i=0; i < A->rows; i++)
                for(j=0; j < A->cols; j++)
                    {
                        if(A_ptr8[ (i*A->cols) + j] > possibleMax) 
                            {
                                possibleMax = A_ptr8[ (i*A->cols) + j];
                                (*row) = i;
                                (*col) = j;
                            }
                    }
        }
    return possibleMax;
}

double avg(CYCLOPS_Matrix* A)
{
    double average;
    uint32_t sum=0;
    uint16_t i;
    uint16_t size=(A->cols) * (A->rows);
    if(A->depth==CYCLOPS_1BYTE)
        {
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
		{
			return -EINVAL;
		}			
            for(i=0;i<size;i++)
                    sum+= A_ptr8[i];
        }
    else if(A->depth==CYCLOPS_2BYTE)
        {		
		uint16_t *A_ptr16;
		A_ptr16 = ker_get_handle_ptr (A->data.hdl16);
		if ( A_ptr16 == NULL) 
		{
			return -EINVAL;
		}		
            for(i=0;i<size;i++)
                sum+=A_ptr16[i];
        }
    else
        {
            return 0;
        }
    average=((double)sum) / size;
    return average;
}
