/**
 * \file DVMBuffer.c
 * \brief DVM Buffer Library Routines
 */

#include <VM/DVMBuffer.h>

//--------------------------------------------------------------------------------------------
static void buffer_get(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint16_t *dest) 
{
  uint16_t res = 0;
  DEBUG("BUFFER_GET: offset %d numBytes %d size %d.\n", bufferOffset, numBytes, buffer->size);
  DEBUG("BUFFER entries %d %d \n", buffer->entries[0], buffer->entries[1]);
  if ((bufferOffset + numBytes) <= buffer->size) {
    if (numBytes == 2) {
      res = buffer->entries[bufferOffset+1];
    }
    *dest = (res << 8) + buffer->entries[bufferOffset];
    DEBUG("BUFFER: Correct place.\n");
  }
  return;
}
//--------------------------------------------------------------------------------------------
static void buffer_set(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint32_t val) 
{
  uint8_t i = 0;
  if ((bufferOffset + numBytes) <= DVM_BUF_LEN) {
    buffer->size = bufferOffset + numBytes;
    for (; i < numBytes; i++) {
      buffer->entries[bufferOffset + i] = val & 0xFF;
      val >>= 8;
    }
  }
  return;
}
//--------------------------------------------------------------------------------------------
static void buffer_clear(DvmDataBuffer *buffer) 
{
  buffer->size = 0;
  return;
}
//--------------------------------------------------------------------------------------------
static void buffer_append(DvmDataBuffer *buffer, uint8_t numBytes, uint16_t var) 
{
  if ((buffer->size + numBytes) <= DVM_BUF_LEN) {	// Append in Little Endian format
    buffer->entries[(int)buffer->size] = var & 0xFF;
    DEBUG("Inside BAPPEND: added 0x%x \n", buffer->entries[(int)buffer->size]);
    buffer->size++;
    if (numBytes == 2) {
      buffer->entries[(int)buffer->size] = var >> 8;
      DEBUG("Inside BAPPEND: added 0x%x \n", buffer->entries[(int)buffer->size]);
      buffer->size++;
    }
  }
  return;
}
//--------------------------------------------------------------------------------------------
static void buffer_concatenate(DvmDataBuffer *dst, DvmDataBuffer *src) 
{
  uint8_t i, start, end, num = 2;
  uint16_t var;
  start = dst->size;
  end = start + src->size;
  end = (end > DVM_BUF_LEN)? DVM_BUF_LEN : end;
  for (i=start; i<end; i+=num) {
    num = ((end - i) >= 2)? 2: (end - i);
    buffer_get(src, num, i - start, &var);
    buffer_append(dst, num, var);
  }
  return;
}
//--------------------------------------------------------------------------------------------  
static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2) 
{
  uint32_t res = (uint16_t)arg1->value.var;
  res <<= 16;
  res += (uint16_t)arg2->value.var;
  return (int32_t)res;
}
//--------------------------------------------------------------------------------------------  
