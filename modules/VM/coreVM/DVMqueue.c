#include <module.h>

#ifdef LINK_KERNEL
//#define _MODULE_
#endif

#include <VM/DVMqueue.h>

#ifndef _MODULE_
#include <sos_list.h>
static inline void list_insert_beforeVM(list_link_t* before, list_link_t* toInsert)
{ list_insert_before(before, toInsert); }

static inline void list_insert_headVM(list_t* list, list_link_t* element) 
{ list_insert_head(list, element); }

static inline void list_removeVM(list_link_t* ll) 
{ list_remove(ll); }

static inline void list_initVM(list_t* list) 
{ list_init(list); }

static inline bool list_emptyVM(list_t* list) 
{ return list_empty(list); }

int8_t dvm_queue_init() 
{ return SOS_OK; }
#else 
static void list_insert_beforeVM(list_link_t* before, list_link_t* toInsert) 
{
	toInsert->l_next = before;
	toInsert->l_prev = before->l_prev;
	before->l_prev->l_next = toInsert;
	before->l_prev = toInsert;
}

static void list_insert_headVM(list_t* list, list_link_t* element) 
{
	list_insert_beforeVM(list->l_next, element);
} 


static void list_removeVM(list_link_t* ll) 
{
	list_link_t *before = ll->l_prev;
	list_link_t *after = ll->l_next;
	if (before->l_next != ll && after->l_prev != ll) 
	{
		ll->l_next = 0;
		ll->l_prev = 0;
		return;
	}
	else if (before->l_next != ll || after->l_prev != ll) 
	{
		DEBUG("VM: ERROR: corrupted queue\n");
		return;
	}
	before->l_next = after;
	after->l_prev = before;
	ll->l_next = 0;
	ll->l_prev = 0;        
}

static void list_initVM(list_t* list) 
{
	list->l_next = list->l_prev = list;
}
	
static bool list_emptyVM(list_t* list) 
{
	return ((list->l_next == list)? 1:0);
}

static int8_t dvm_queue(void *state, Message *msg);
static int8_t queue_init(func_cb_ptr p, DvmQueue* queue);
static uint8_t queue_empty(func_cb_ptr p, DvmQueue* queue);
static int8_t queue_enqueue(func_cb_ptr p, DvmContext* context, DvmQueue* queue, DvmContext* element);
static DvmContext* queue_dequeue(func_cb_ptr p, DvmContext* context, DvmQueue* queue); 
static int8_t queue_remove(func_cb_ptr p, DvmContext* context, DvmQueue* queue, DvmContext* element); 
static const mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id         = M_QUEUE,
	.code_id        = ehtons(M_QUEUE),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,
	.state_size     = 0,                                         
	.num_timers     = 0,                                                     
	.num_sub_func   = 0,                                                     
	.num_prov_func  = 5,                                                     
	.module_handler = dvm_queue,                                  
	.funct          = {                                       
		[0] = PRVD_FUNC_queue_init(M_QUEUE, Q_INIT),        
		[1] = PRVD_FUNC_queue_empty(M_QUEUE, Q_EMPTY),        
		[2] = PRVD_FUNC_queue_enqueue(M_QUEUE, Q_ENQUEUE),        
		[3] = PRVD_FUNC_queue_dequeue(M_QUEUE, Q_DEQUEUE),        
		[4] = PRVD_FUNC_queue_remove(M_QUEUE, Q_REMOVE),        
	},                                                        

};        

static int8_t dvm_queue(void *state, Message *msg)
{                                                             
	    return SOS_OK;                                            
}

#ifdef LINK_KERNEL
int8_t dvm_queue_init() 
{
	    return ker_register_module(sos_get_header_address(mod_header));
}
#endif

#endif

#ifndef _MODULE_
int8_t queue_init(DvmQueue* queue) 
#else
static int8_t queue_init(func_cb_ptr p, DvmQueue* queue) 
#endif
{
	DEBUG("VM: Initializing queue %p\n", queue);
	list_initVM(&queue->queue);
	return SOS_OK;
}

#ifndef _MODULE_
uint8_t queue_empty(DvmQueue* queue) 
#else
static uint8_t queue_empty(func_cb_ptr p, DvmQueue* queue) 
#endif
{
	bool emp = list_emptyVM(&queue->queue);
	DEBUG("VM: Testing if queue at %p is empty: %s.\n", queue, (emp)? "true":"false");
	return emp;
}

#ifndef _MODULE_
int8_t queue_enqueue(DvmContext* context, DvmQueue* queue, DvmContext* element) 
#else
static int8_t queue_enqueue(func_cb_ptr p, DvmContext* context, DvmQueue* queue, DvmContext* element) 
#endif
{
	DEBUG("VM (%i): Enqueue %i on %p...", (int)context->which, (int)element->which, queue);
	if (element->queue) 
	{
		//error_error(error_proto, context, DVM_ERROR_QUEUE_ENQUEUE);
		DEBUG_SHORT("FAILURE, already there\n");
		return -EINVAL;
	}
	element->queue = queue;
	list_insert_headVM(&queue->queue, &element->link);
	DEBUG_SHORT("success\n"); 
	return SOS_OK;
}


#ifndef _MODULE_
DvmContext* queue_dequeue(DvmContext* context, DvmQueue* queue) 
#else
static DvmContext* queue_dequeue(func_cb_ptr p, DvmContext* context, DvmQueue* queue) 
#endif
{
	DvmContext* rval;
	list_link_t* listLink;;

	if (list_emptyVM(&queue->queue)) 
	{
		//error_error(error_proto, context, DVM_ERROR_QUEUE_DEQUEUE);
		return NULL;
	}

	listLink = queue->queue.l_prev;
	rval = (DvmContext*)((char*)listLink - offsetof(DvmContext, link));
	list_removeVM(listLink);
	rval->link.l_next = 0;
	rval->link.l_prev = 0;
	rval->queue = NULL;
	if (rval != NULL) 
	{
		DEBUG("VM: Dequeuing context %i from queue %p.\n", (int)rval->which, queue);
	}
	return rval;
}

#ifndef _MODULE_
int8_t queue_remove(DvmContext* context, DvmQueue* queue, DvmContext* element) 
#else
static int8_t queue_remove(func_cb_ptr p, DvmContext* context, DvmQueue* queue, DvmContext* element) 
#endif
{
	DEBUG("VM (%i): Removing context %i from queue %p.\n", (int)context->which, (int)element->which, queue);
	if (element->queue != queue) 
	{
		return -EINVAL;
	}
	element->queue = NULL;
	if (!(element->link.l_next && element->link.l_prev)) 
	{
		return -EINVAL;
	}
	else 
	{
		list_removeVM(&element->link);
		return SOS_OK;
	}
}

