#ifndef _DVM_QUEUE_H_INCL_
#define _DVM_QUEUE_H_INCL_

#include <VM/Dvm.h>
#include <sos_list.h>

int8_t queue_init(DvmQueue* queue);

uint8_t queue_empty(DvmQueue* queue);

int8_t queue_enqueue(DvmContext* context, DvmQueue* queue, DvmContext* element);

DvmContext* queue_dequeue(DvmContext* context, DvmQueue* queue);

int8_t queue_remove(DvmContext* context, DvmQueue* queue, DvmContext* element);

#endif//_DVM_QUEUE_H_INCL_
