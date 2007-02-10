/**
 * \file DVMqueue.c
 * \brief DVM Queue Implementation
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/DVMqueue.h>
#ifdef PC_PLATFORM
#include <sos_sched.h>
#endif

//--------------------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------------------
static void list_insert_beforeVM(list_link_t* before, list_link_t* toInsert) 
{
  toInsert->l_next = before;
  toInsert->l_prev = before->l_prev;
  before->l_prev->l_next = toInsert;
  before->l_prev = toInsert;
}
//--------------------------------------------------------------------
static void list_insert_headVM(list_t* list, list_link_t* element) 
{
  list_insert_beforeVM(list->l_next, element);
} 
//--------------------------------------------------------------------
static void list_removeVM(list_link_t* ll) 
{
  list_link_t *before = ll->l_prev;
  list_link_t *after = ll->l_next;
  if (before->l_next != ll && after->l_prev != ll) {
    ll->l_next = 0;
    ll->l_prev = 0;
    return;
  }
  else if (before->l_next != ll || after->l_prev != ll) {
    DEBUG("VM: ERROR: corrupted queue\n");
    return;
  }
  before->l_next = after;
  after->l_prev = before;
  ll->l_next = 0;
  ll->l_prev = 0;        
}
//--------------------------------------------------------------------
static void list_initVM(list_t* list) 
{
  list->l_next = list->l_prev = list;
}
//--------------------------------------------------------------------	
static bool list_emptyVM(list_t* list) 
{
  return ((list->l_next == list)? 1:0);
}
//--------------------------------------------------------------------
// QUEUE LIBRARY
//--------------------------------------------------------------------
int8_t queue_init(DvmQueue* queue) 
{
  DEBUG("VM: Initializing queue %p\n", queue);
  list_initVM(&queue->queue);
  return SOS_OK;
}
//--------------------------------------------------------------------
uint8_t queue_empty(DvmQueue* queue) 
{
  bool emp = list_emptyVM(&queue->queue);
  DEBUG("VM: Testing if queue at %p is empty: %s.\n", queue, (emp)? "true":"false");
  return emp;
}
//--------------------------------------------------------------------
int8_t queue_enqueue(DvmContext* context, DvmQueue* queue, DvmContext* element) 
{
  DEBUG("VM (%i): Enqueue %i on %p...", (int)context->which, (int)element->which, queue);
  if (element->queue) {
    //error_error(error_proto, context, DVM_ERROR_QUEUE_ENQUEUE);
    DEBUG_SHORT("FAILURE, already there\n");
    return -EINVAL;
  }
  element->queue = queue;
  list_insert_headVM(&queue->queue, &element->link);
  DEBUG_SHORT("success\n"); 
  return SOS_OK;
}
//--------------------------------------------------------------------
DvmContext* queue_dequeue(DvmContext* context, DvmQueue* queue) 
{
  DvmContext* rval;
  list_link_t* listLink;;
  if (list_emptyVM(&queue->queue)){
    //error_error(error_proto, context, DVM_ERROR_QUEUE_DEQUEUE);
    return NULL;
  }
  listLink = queue->queue.l_prev;
  rval = (DvmContext*)((char*)listLink - offsetof(DvmContext, link));
  list_removeVM(listLink);
  rval->link.l_next = 0;
  rval->link.l_prev = 0;
  rval->queue = NULL;
  if (rval != NULL) {
    DEBUG("VM: Dequeuing context %i from queue %p.\n", (int)rval->which, queue);
  }
  return rval;
}
//--------------------------------------------------------------------
int8_t queue_remove(DvmContext* context, DvmQueue* queue, DvmContext* element) 
{
  DEBUG("VM (%i): Removing context %i from queue %p.\n", (int)context->which, (int)element->which, queue);
  if (element->queue != queue) {
    return -EINVAL;
  }
  element->queue = NULL;
  if (!(element->link.l_next && element->link.l_prev)) {
    return -EINVAL;
  }
  else {
    list_removeVM(&element->link);
    return SOS_OK;
  }
}
//--------------------------------------------------------------------
