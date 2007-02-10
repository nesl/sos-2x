/**
 * \file DVMStacks.c
 * \brief Dynamic Virtual Machine Stacks
 * \author Rahul Balani
 * \author Ilias Tsigkogiannis
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/DVMStacks.h>
#include <VM/DVMScheduler.h> // For error function
#ifdef PC_PLATFORM
#include <sos_sched.h>
#endif

//-------------------------------------------------------------------
int8_t resetStacks(DvmState *eventState) 
{
  eventState->stack.sp = 0;
  return SOS_OK;
}
//-------------------------------------------------------------------
int8_t pushValue(DvmState *eventState, int16_t val, uint8_t type) 
{
  DvmContext* context = &(eventState->context);
  if (eventState->stack.sp >= DVM_OPDEPTH) {
    DEBUG("VM: Tried to push value off end of stack.\n");
    error(context, DVM_ERROR_STACK_OVERFLOW);
    return -EINVAL;
  }
  else {
    uint8_t sIndex = eventState->stack.sp;
    eventState->stack.stack[(int)sIndex].type = type;
    eventState->stack.stack[(int)sIndex].value.var = val;
    eventState->stack.sp++;
    return SOS_OK;
  }
}
//-------------------------------------------------------------------
int8_t pushBuffer(DvmState *eventState, DvmDataBuffer* buffer) 
{
  DvmContext* context = &(eventState->context);
  if (eventState->stack.sp >= DVM_OPDEPTH) {
    DEBUG("VM: Tried to push value off end of stack.\n");
    error(context, DVM_ERROR_STACK_OVERFLOW);
    return -EINVAL;
  }
  else {
    uint8_t sIndex = eventState->stack.sp;
    eventState->stack.stack[(int)sIndex].type = DVM_TYPE_BUFFER;
    eventState->stack.stack[(int)sIndex].buffer.var = buffer;
    eventState->stack.sp++;
    return SOS_OK;
  }
  return SOS_OK;
}
//-------------------------------------------------------------------
int8_t pushOperand(DvmState *eventState, DvmStackVariable* var) 
{
  DvmContext* context = &(eventState->context);
  if (eventState->stack.sp >= DVM_OPDEPTH) {
    DEBUG("VM: Tried to push value off end of stack.\n");
    error(context, DVM_ERROR_STACK_OVERFLOW);
    return -EINVAL;
  }
  else {
    uint8_t sIndex = eventState->stack.sp;
    eventState->stack.stack[(int)sIndex] = *var;
    eventState->stack.sp++;
    return SOS_OK;
  }
  return SOS_OK;
}
//-------------------------------------------------------------------
DvmStackVariable* popOperand(DvmState *eventState) 
{
  DvmStackVariable* var;
  DvmContext* context = &(eventState->context);  
  if (eventState->stack.sp == 0) {
    DEBUG("VM: Tried to pop off end of stack.\n");
    eventState->stack.stack[0].type = DVM_TYPE_NONE;
    error(context, DVM_ERROR_STACK_UNDERFLOW);
    return &(eventState->stack.stack[0]);
  }
  else {
    uint8_t sIndex;
    eventState->stack.sp--;
    sIndex = eventState->stack.sp;
    var = &(eventState->stack.stack[(int)sIndex]);
    return var;
  }
  return NULL;
}
//-------------------------------------------------------------------
