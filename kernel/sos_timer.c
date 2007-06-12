/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/** 
 * @brief delta timer implementation
 * @author Simon Han
 * @brief Pre-allocated timers with safe blocks
 * @author Ram Kumar
 */

#include <sos_timer.h>
#include <message_queue.h>
#include <malloc.h>
#include <hardware_types.h>
#include <timer.h>
#include <message.h>
#include <measurement.h>
#include <sos_sched.h>
#include <sos_info.h>
#include <sos_list.h>
#include <sos_logging.h>
#include <slab.h>

#ifndef SOS_DEBUG_TIMER
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_SHORT
#define DEBUG_SHORT(...)
#define print_all_timers(...)
#endif

#define MAX_SLEEP_INTERVAL 250
#define MAX_REALTIME_CLOCK 4

//------------------------------------------------------------------------
// INTERNAL DATA STRUCTURE
//------------------------------------------------------------------------
typedef struct {
	uint16_t value;
	uint16_t interval;
	timer_callback_t f;
} timer_realtime_t;

//------------------------------------------------------------------------
// GLOBAL VARIABLES
//------------------------------------------------------------------------
static list_t   deltaq;              //!< Timer delta queue
static list_t   timer_pool;          //!< Pool of initialized timers
static list_t   prealloc_timer_pool; //!< Pool of pre-allocated timers
static list_t  periodic_pool;        //!< periodic pool used by soft_interrupt
static int32_t  outstanding_ticks = 0; 

static uint8_t num_realtime_clock = 0;
static timer_realtime_t realtime[MAX_REALTIME_CLOCK];
static slab_t timer_slab;

//------------------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//------------------------------------------------------------------------
static void timer_delta_q_insert(sos_timer_t *h, bool new_timer);
static sos_timer_t* find_timer_block(sos_pid_t pid, uint8_t tid);
static sos_timer_t *find_timer_in_periodic_pool(sos_pid_t pid, uint8_t tid);
static sos_timer_t* alloc_from_timer_pool(sos_pid_t pid, uint8_t tid);
static sos_timer_t* alloc_from_preallocated_timer_pool(sos_pid_t pid);
static int8_t timer_remove_timer(sos_timer_t *tt);
static void timer_update_delta(void);
static void timer_pre_alloc_block_init(sos_timer_t *h, sos_pid_t pid);
static uint16_t timer_update_realtime_clock(uint8_t cnt);


//------------------------------------------------------------------------
// KERNEL FUNCTIONS
//------------------------------------------------------------------------

/**
 * @brief Initialize the timer unit
 */
void timer_init(void)
{
	uint8_t i;
  list_init(&deltaq);
  list_init(&timer_pool);
  list_init(&prealloc_timer_pool);
  list_init(&periodic_pool);

  for(i = 0; i < MAX_REALTIME_CLOCK; i++) {
	realtime[i].f = NULL;  
  }
  
  ker_slab_init( TIMER_PID, &timer_slab, sizeof(sos_timer_t), 4, SLAB_LONGTERM );
}


/**
 * @brief Pre-allocate timers for a module at load time
 */
int8_t timer_preallocate(sos_pid_t pid, uint8_t num_timers)
{
   // We have already checked if num_timer > 0 and pid is not NULL_PID

  uint8_t i, j;
  sos_timer_t* tt[MAX_PRE_ALLOCATED_TIMERS];
   
  //! We cannot allow a single module to pre allocate a lot of timers
  if (num_timers > MAX_PRE_ALLOCATED_TIMERS)
	return -EINVAL;
  
  //! First try to safely allocate memory blocks for all the pre-allocated timers
  for (i = 0; i < num_timers; i++){
	tt[i] = (sos_timer_t*)ker_slab_alloc(&timer_slab, TIMER_PID);
	if (tt[i] == NULL){
	  for (j = 0; j < i; j++){
		ker_slab_free(&timer_slab, tt[j]);
	  }
	  return -ENOMEM;
	}   
  }
  
   //! If we get here then we have all the memory allocated
   //! Now initialize all the data structures and just add them to the timer pool
  for (i = 0; i < num_timers; i++){
	timer_pre_alloc_block_init(tt[i], pid);
  }
  
  return SOS_OK;
}

/**
 * @brief remove timers for a particular pid
 */
int8_t timer_remove_all(sos_pid_t pid)
{
  list_link_t *link;
  
  for(link = deltaq.l_next;
	  link != (&deltaq); link = link->l_next) {
	sos_timer_t *h = (sos_timer_t*)link;         
	if(h->pid == pid) {
	  link = link->l_prev;
	  timer_remove_timer(h);
	  ker_slab_free( &timer_slab, h );
	  //	break; Ram - Why are we breaking from this loop ?
	}
  }
	
  for (link = timer_pool.l_next; link != (&timer_pool); link = link->l_next){
	sos_timer_t *h = (sos_timer_t*)link;
	if (h->pid == pid){
	  link = link->l_prev;
	  list_remove((list_link_t*)h);
	  ker_slab_free(&timer_slab,h);
	}
  }

  for (link = prealloc_timer_pool.l_next; link != (&prealloc_timer_pool); link = link->l_next){
	sos_timer_t *h = (sos_timer_t*)link;
	if (h->pid == pid){
	  link = link->l_prev;
	  list_remove((list_link_t*)h);
	  ker_slab_free(&timer_slab,h);
	}
  }

  for (link = periodic_pool.l_next; link != (&periodic_pool); link = link->l_next){
	sos_timer_t *h = (sos_timer_t*)link;
	if (h->pid == pid){
	  link = link->l_prev;
	  list_remove((list_link_t*)h);
	  ker_slab_free(&timer_slab,h);
	}
  }

  return SOS_OK;
}


//------------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------------

/**
 * @brief Initialize the pre-allocated timer blocks
 */ 
static void timer_pre_alloc_block_init(sos_timer_t *h, sos_pid_t pid)
{
   h->pid = pid;                  //! Indicate ownership of a timer block
   h->flag = TIMER_PRE_ALLOCATED; //! Pre-allocated timer block
   list_insert_tail(&prealloc_timer_pool, (list_link_t*)h);
}

static void timer_set_hw_interval(int32_t cnt)
{
	if( cnt < TIMER_MIN_INTERVAL ) {
		DEBUG("set hw top to %d\n", TIMER_MIN_INTERVAL );
		timer_setInterval(TIMER_MIN_INTERVAL);
	} else if( cnt > MAX_SLEEP_INTERVAL ) {
		DEBUG("set hw top to %d\n", MAX_SLEEP_INTERVAL);
		timer_setInterval(MAX_SLEEP_INTERVAL);
	} else {
		DEBUG("set hw top to %d\n", cnt);
		timer_setInterval((uint8_t)cnt);
	}
}

static void timer_set_hw_top(int32_t cnt, bool update_outstanding)
{
	uint8_t hw_cnt = timer_hardware_get_counter();
	uint16_t rt_cnt;
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();
	if( update_outstanding ) {
		outstanding_ticks += hw_cnt;
	} else {
		outstanding_ticks = 0;
	}
	if( num_realtime_clock > 0 ) {
		rt_cnt = timer_update_realtime_clock(hw_cnt);

		if( rt_cnt < cnt ) {
			cnt = rt_cnt;
		}
	}

	timer_set_hw_interval(cnt);

	LEAVE_CRITICAL_SECTION();
}

#ifdef SOS_DEBUG_TIMER
static void print_all_timers(char *context)
{
	list_link_t *link;
	uint8_t i = 0;

	DEBUG(" *** ALL TIMER: %s ***\n", context);
	for(link = deltaq.l_next;
			link != (&deltaq); link = link->l_next, i++) {
		sos_timer_t *h = (sos_timer_t*)link;
		DEBUG("(%d) pid = %d, tid = %d, ticks = %d, delta = %d, prev = %x, next = %x\n", i, h->pid, h->tid, h->ticks, h->delta, (int)h->list.l_prev, (int)h->list.l_next);
	}
}
#endif

/**
 * @brief insert handle into delta queue
 * This routine assumes that the data structure is set
 */
static void timer_delta_q_insert(sos_timer_t *h, bool new_timer)
{
	list_link_t *link;
	int32_t hw_cnt;
	HAS_CRITICAL_SECTION;
	
	DEBUG("ticks = %d, delta = %d\n", h->ticks, h->delta);
	if(list_empty(&deltaq) == true) {
		//! empty queue
		//! start the timer
		DEBUG("empty q, set top to %d\n", h->delta);
		if( new_timer ) {
			// clear any outstnading ticks
			// and start new timer
			timer_set_hw_top(h->delta, false);
		}
		list_insert_head(&deltaq, (list_link_t*)h);
		return;
	}

	ENTER_CRITICAL_SECTION();
	hw_cnt = outstanding_ticks + timer_hardware_get_counter();
	LEAVE_CRITICAL_SECTION();

	if( new_timer ) {
		// if it is a new timer, we need to add the ticks that are 
		// already counted because these ticks will be subtracted 
		// later.
		// outstanding_ticks + timer_hardware_get_counter() is the 
		// ticks that are already passed in time
		h->delta += hw_cnt;
		DEBUG("get hw_cnt = %d\n", hw_cnt);
	}

	link = deltaq.l_next;

	// Check whether new timer will be new head
	// because we need to modify hardware counter if it is the case
	if( h->delta < (((sos_timer_t*)link)->delta)) {
		DEBUG("new timer will be the head\n");
		(((sos_timer_t*)link)->delta) -= (h->delta);
		if( new_timer ) {
			timer_set_hw_top(h->delta - hw_cnt, true);
		}
		list_insert_head(&deltaq, (list_link_t*)h);
		return;
	}

	// Work this timer to the current position
	for(/* initialized already */ ;
		link != (&deltaq); 
		link = link->l_next) {
		sos_timer_t *curr = (sos_timer_t*)link;
		if(h->delta < curr->delta) {
			//! insert here
			DEBUG("insert to middle\n");
			curr->delta -= h->delta;
			list_insert_before(link, (list_link_t*)h);
			return;
		}
		h->delta -= curr->delta;
	}
	DEBUG("insert to tail\n");
	list_insert_tail(&deltaq, (list_link_t*)h);
	return;
}
/**
 * @brief Locate a timer block from the detlaq
 */
static sos_timer_t* find_timer_block(sos_pid_t pid, uint8_t tid)
{
   sos_timer_t* tt;
   

   if (list_empty(&deltaq)){
      return NULL;   
   }
   
   tt = (sos_timer_t*) deltaq.l_next;
   do{
      if ((tt->pid == pid) &&
          (tt->tid == tid))
         return tt;
      tt = (sos_timer_t*)tt->list.l_next;
   } while ((list_t*)tt != &deltaq);

   return NULL;
}

static sos_timer_t *find_timer_in_periodic_pool(sos_pid_t pid, uint8_t tid)
{
   sos_timer_t* tt;

   if (list_empty(&periodic_pool) == false) {
	   tt = (sos_timer_t*) periodic_pool.l_next;
	   do {
		   if( (tt->pid == pid) && (tt->tid == tid) ) {
				list_remove((list_t*)tt);
				return tt;
		   }
		   tt = (sos_timer_t*)tt->list.l_next;
	   } while((list_t*)tt != &periodic_pool);
   }
   return NULL;
}

/**
 * @brief Locate a free timer block from the preallocated timer pool
 */
static sos_timer_t* alloc_from_preallocated_timer_pool(sos_pid_t pid)
{
   sos_timer_t* tt;
   
   if (list_empty(&prealloc_timer_pool))
      return NULL;
   
   //! Find an unused pre-allocated block or an intialized block
   tt = (sos_timer_t*) prealloc_timer_pool.l_next;
   do{
	 
      if (tt->pid == pid)
		{
		  list_remove((list_t*)tt);
		  return tt;
		}
	  tt = (sos_timer_t*)tt->list.l_next;
   } while ((list_t*)tt != &prealloc_timer_pool);
   
   return NULL;
}


static sos_timer_t* alloc_from_timer_pool(sos_pid_t pid, uint8_t tid)
{
   sos_timer_t* tt;
   
   if (list_empty(&timer_pool)) {
      return NULL;
   }
   
   //! Find an unused pre-allocated block or an intialized block
   tt = (sos_timer_t*) timer_pool.l_next;
   do{
	 
      if ((tt->pid == pid) &&
          (tt->tid == tid))
		{
		  list_remove((list_t*)tt);
		  return tt;
		}
	  tt = (sos_timer_t*)tt->list.l_next;
   } while ((list_t*)tt != &timer_pool);
   
   return NULL;
}

/**
 * @brief Remove a timer from the deltaq
 */
static int8_t timer_remove_timer(sos_timer_t *tt)
{
	
	if((tt->list.l_next == NULL) ||
	   (tt->list.l_prev == NULL)) {
		return -EINVAL;
	}
	// if I am not the tail... and I have positive delta
	if((tt->list.l_next != &deltaq) && (tt->delta > 0 )) {
		((sos_timer_t*)(tt->list.l_next))->delta += tt->delta;
	}
	list_remove((list_t*)tt);
    return SOS_OK;
}

/**
 * @brief update delta queue
 * traverse each item in the queue until no more delta left
 * NOTE: this is executed in interrupt handler, so NO lock necessary
 */
static void timer_update_delta(void)
{
	list_link_t *link;
	int32_t delta;
	HAS_CRITICAL_SECTION;
	
	ENTER_CRITICAL_SECTION();
	delta = outstanding_ticks;
	outstanding_ticks = 0;
	LEAVE_CRITICAL_SECTION();
	
	if(list_empty(&deltaq) == true) {
		return;
	}
	DEBUG("update delta = %d\n", delta);
	for(link = deltaq.l_next;
			link != (&deltaq); link = link->l_next) {
		sos_timer_t *h = (sos_timer_t*)link;         
		if(h->delta >= delta) {
			// if we use all ticks...
			h->delta -= delta;
			return;
		} else {
			int32_t tmp = h->delta;
			h->delta -= delta;
			delta -= tmp;
		}
	}
}

/**
 * @brief Post the timeout messages
 */



//------------------------------------------------------------------------
// TIMER API
//------------------------------------------------------------------------

//! Assumption - This function is never called from an interrupt context
int8_t ker_timer_init(sos_pid_t pid, uint8_t tid, uint8_t type)
{
  sos_timer_t* tt;

  tt = find_timer_in_periodic_pool(pid, tid);
  if (tt != NULL) {
      tt->type = type;
	  list_insert_tail(&timer_pool, (list_link_t*)tt);
	  return SOS_OK;
  }
  //! re-initialize an existing timer by stoping it and updating the type
  tt = find_timer_block(pid, tid);
  if (tt != NULL){
      ker_timer_stop(pid,tid);
      tt->type = type;
      return SOS_OK;
  }
  
  //! Search if pre-initialized timer exists
  tt = alloc_from_timer_pool(pid, tid);
  
  //! Look for pre-allocated timers or try to get dynamic memory
  if (tt == NULL){
	tt = alloc_from_preallocated_timer_pool(pid);
	if (tt == NULL)
	  tt = (sos_timer_t*)ker_slab_alloc(&timer_slab, TIMER_PID);
	//! Init will fail if the system does not have sufficient resources
	if (tt == NULL)
	  return -ENOMEM;
  }
  
  //! Fill up the data structure and insert into the timer pool
  tt->pid = pid;
  tt->tid = tid;
  tt->type = type;
  
  list_insert_tail(&timer_pool, (list_link_t*)tt);
  return SOS_OK;
}

int8_t ker_permanent_timer_init(sos_timer_t* tt, sos_pid_t pid, uint8_t tid, uint8_t type)
{
   //! Fill up the data structures and insert into the timer pool
   tt->pid = pid;
   tt->tid = tid;
   tt->type = type | PERMANENT_TIMER_MASK;
   list_insert_tail(&timer_pool, (list_link_t*)tt);

   return SOS_OK; 
}

void timer_gc( void )
{
	list_link_t *link;
	
	for(link = deltaq.l_next; link != (&deltaq); link = link->l_next) {
		if( (((sos_timer_t*)link)->type & PERMANENT_TIMER_MASK) == 0 ) {
			slab_gc_mark( &timer_slab, link );
		} 
	}
	
	for (link = timer_pool.l_next; link != (&timer_pool); link = link->l_next) {
		if( (((sos_timer_t*)link)->type & PERMANENT_TIMER_MASK) == 0 ) {
			slab_gc_mark( &timer_slab, link );
		}
	}
	
	for (link = prealloc_timer_pool.l_next; link != (&prealloc_timer_pool); link = link->l_next) {
		if( (((sos_timer_t*)link)->type & PERMANENT_TIMER_MASK) == 0 ) {
			slab_gc_mark( &timer_slab, link );
		}
	}
	
	for (link = periodic_pool.l_next; link != (&periodic_pool); link = link->l_next) {
		if( (((sos_timer_t*)link)->type & PERMANENT_TIMER_MASK) == 0 ) {
			slab_gc_mark( &timer_slab, link );
		}
	}
	
	slab_gc( &timer_slab, TIMER_PID );
	
	malloc_gc( TIMER_PID );
}


int8_t ker_timer_start(sos_pid_t pid, uint8_t tid, int32_t interval)
{
  sos_timer_t* tt;
  //! Start the timer from the timer pool
  tt = alloc_from_timer_pool(pid, tid);
  
  //! If the timer does not exist, then it is already in use or not initialized
  if (tt == NULL) {
	DEBUG_PID(TIMER_PID, "ker_timer_start: tt == NULL\n");
	return -EINVAL;
  }
  
  //  tt->ticks = PROCESSOR_TICKS(interval);
  tt->ticks = interval;
  tt->delta = interval;
  
  //DEBUG("timer_start(%d) %d %d %d\n", tt->pid, tt->tid, tt->type, tt->ticks);
  
  //! insert into delta queue
  print_all_timers("timer_start_start");
  timer_delta_q_insert(tt, true);
  print_all_timers("timer_start_end");
  ker_log( SOS_LOG_TIMER_START, pid, tid );
  return SOS_OK;
}



//! The implementation of this call can be optimized to include the find with
//! the remove. We will do it later
int8_t ker_timer_stop(sos_pid_t pid, uint8_t tid)
{
  sos_timer_t* tt;

  tt = find_timer_in_periodic_pool(pid, tid);
  if( tt == NULL ) {
	  //! Find the timer block
	  tt = find_timer_block(pid, tid);
	  if (tt == NULL) {
		  return -EINVAL;
	  } else {
		  //! Remove the timer from the deltaq and any pending messages in the queue
		  timer_remove_timer(tt);
	  }
  }
  //timer_remove_timeout_from_scheduler(tt);
  //! Re-insert timer into the pool
  list_insert_tail(&timer_pool, (list_link_t*)tt);
  ker_log( SOS_LOG_TIMER_STOP, pid, tid );
  return SOS_OK;
}

//! Free the first timer block beloning to pid in the timer_pool
int8_t ker_timer_release(sos_pid_t pid, uint8_t tid)
{
  sos_timer_t* tt;

  //! First stop the timer if it is running
  ker_timer_stop(pid, tid);

  //! Get the timer block from the pool
  tt = alloc_from_timer_pool(pid, tid);
  
  if (tt == NULL) 
	return -EINVAL;

  //! Deep free of the timer
  ker_slab_free(&timer_slab,tt); 
  
  return SOS_OK;   
}


int8_t ker_timer_restart(sos_pid_t pid, uint8_t tid, int32_t interval)
{
  sos_timer_t* tt;
  
  tt = find_timer_in_periodic_pool(pid, tid);
  if (tt == NULL) {
	  //! Locate a running timer or from the timer pool
	  tt = find_timer_block(pid, tid);
	  if (tt != NULL){
		  timer_remove_timer(tt);
	  }
	  else {
		  tt = alloc_from_timer_pool(pid, tid);
	  }
  }
   
   //! The timer is neither running nor initialized
  if (tt == NULL) {
	return -EINVAL;
  }
  /* Special Case restart with existing ticks field */
  if( interval <= 0 )
      interval = tt->ticks;

  if(interval < TIMER_MIN_INTERVAL){
      /* Need to put the timer back in to the pool 
         as an initialized timer that was never started */
      list_insert_tail(&timer_pool, (list_link_t*)tt);
      return -EPERM;
  }
  //! Initialize the data structure
  tt->ticks = interval;
  //  tt->ticks = PROCESSOR_TICKS(interval);
  tt->delta = interval;

  //! Insert into the delta queue
  timer_delta_q_insert(tt, true);
  ker_log( SOS_LOG_TIMER_RESTART, pid, tid );
  return SOS_OK;
}


int8_t ker_sys_timer_start(uint8_t tid, int32_t interval, uint8_t type)
{                                                             
	sos_pid_t my_id = ker_get_current_pid();                  

	if( (ker_timer_init(my_id, tid, type) != SOS_OK) ||       
			(ker_timer_start(my_id, tid, interval) != SOS_OK)) {  
		return ker_mod_panic(my_id);                                 
	}                                                         
	return SOS_OK;                                            
}                                                             

int8_t ker_sys_timer_restart(uint8_t tid, int32_t interval)       
{                                                             
	sos_pid_t my_id = ker_get_current_pid();                  
	if( ker_timer_restart(my_id, tid, interval) != SOS_OK ) { 
		return ker_mod_panic(my_id);                                 
	}                                                         
	return SOS_OK;                                            
}                                                             

int8_t ker_sys_timer_stop(uint8_t tid)              
{                                                             
	sos_pid_t my_id = ker_get_current_pid();                  

	ker_timer_stop(my_id, tid);
	ker_timer_release(my_id, tid);
	return SOS_OK;                                            
}                   	

// called from scheduler
static void soft_interrupt( void )
{
	HAS_CRITICAL_SECTION;

	timer_update_delta();
	while(list_empty(&deltaq) == false) {
		sos_timer_t *h = (sos_timer_t*)(deltaq.l_next);         
		if(h->delta <= 0) {
			sos_pid_t pid = h->pid;
			uint8_t tid = h->tid;
			uint8_t flag;
			list_remove_head(&deltaq);

			if(((h->type) & SLOW_TIMER_MASK) == 0){
				flag = SOS_MSG_HIGH_PRIORITY;
			} else {
				flag = 0;
			}

			if (((h->type) & ONE_SHOT_TIMER_MASK) == 0){
				//! periocic timer
				while(h->delta <= 0) {
					// make sure it is positive
					h->delta += h->ticks;
				}
				list_insert_tail(&periodic_pool, (list_t*) h);

			} else {
				list_insert_tail(&timer_pool, (list_link_t*)h);
			}
			sched_dispatch_short_message(pid, TIMER_PID,
					MSG_TIMER_TIMEOUT, 
					tid, 0,
					flag);

		} else {
			break;
		}
	}

	while(list_empty(&periodic_pool) == false) {
		list_link_t *link = periodic_pool.l_next;
		list_remove_head(&periodic_pool);
		timer_delta_q_insert((sos_timer_t*)link, false);
	}

	if(list_empty(&deltaq) == false) {
		sos_timer_t *h = (sos_timer_t*)(deltaq.l_next);
		int32_t hw_cnt;
		ENTER_CRITICAL_SECTION();
		hw_cnt = outstanding_ticks - timer_hardware_get_counter();
		if( h->delta - hw_cnt > 0) {
			LEAVE_CRITICAL_SECTION();
			timer_set_hw_top(h->delta - hw_cnt, true);	
		} else {
			LEAVE_CRITICAL_SECTION();
			sched_add_interrupt(SCHED_TIMER_INT, soft_interrupt);
		}
	} else {
		ENTER_CRITICAL_SECTION();
		timer_set_hw_top(MAX_SLEEP_INTERVAL, false);
		LEAVE_CRITICAL_SECTION();
	}
}

static void timer_realtime_set_hw_top(uint16_t value)
{
	// compute the time it takes to have next interrupt
	if(list_empty(&deltaq) == true) {
		timer_set_hw_top(value, false);
	} else {
		uint8_t hw_interval = timer_getInterval();
		uint8_t hw_cnt = timer_hardware_get_counter();
		if( (hw_interval - hw_cnt) >= value ) {
			if(list_empty(&deltaq) == true) {
				timer_set_hw_top(value, false);
			} else {
				timer_set_hw_top(value, true);
			}
		}
	}
}



int8_t timer_realtime_start(uint16_t value, uint16_t interval, timer_callback_t f)
{
	uint8_t i;
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();
	if( num_realtime_clock == MAX_REALTIME_CLOCK ) {
		LEAVE_CRITICAL_SECTION();
		return -ENOMEM;
	}

	for( i = 0; i < MAX_REALTIME_CLOCK; i++ ) {
		if( realtime[i].f == NULL ) {

			timer_realtime_set_hw_top(value);
			num_realtime_clock++;

			realtime[i].value = value;
			realtime[i].interval = interval;
			realtime[i].f = f;
			LEAVE_CRITICAL_SECTION();
			return SOS_OK;
		}
	}	
	LEAVE_CRITICAL_SECTION();
	return -ENOMEM;
}

int8_t timer_realtime_stop(timer_callback_t f)
{
	uint8_t i;
	HAS_CRITICAL_SECTION;

	ENTER_CRITICAL_SECTION();
	for( i = 0; i < MAX_REALTIME_CLOCK; i++ ) {
		if( realtime[i].f == f ) {
			realtime[i].f = NULL;
			num_realtime_clock--;
			LEAVE_CRITICAL_SECTION();
			return SOS_OK;
		}
	}
	LEAVE_CRITICAL_SECTION();
	return -EINVAL;
}

static uint16_t timer_update_realtime_clock(uint8_t cnt)
{
	uint8_t i;
	uint16_t min_cnt = 65535;
	timer_callback_t f[MAX_REALTIME_CLOCK];

	// iterate through all realtime clock
	for( i = 0; i < MAX_REALTIME_CLOCK; i++ ) {
		f[i] = NULL;
		if( realtime[i].f != NULL ) {
			if( realtime[i].value <= cnt ) {
				f[i] = realtime[i].f;
				if( realtime[i].interval != 0 ) {
					realtime[i].value += (realtime[i].interval - cnt);
				} else {
					realtime[i].f = NULL;
					num_realtime_clock--;
					continue;
				}
			} else {
				realtime[i].value -= cnt;
			}
			if( realtime[i].value < min_cnt ) {                 
				min_cnt = realtime[i].value;                    
			}
		}
	}

	for( i = 0; i < MAX_REALTIME_CLOCK; i++ ) {
		if( f[i] != NULL ) {
			(f[i])();
		}
	}

	return min_cnt;
}

timer_interrupt()
{
	uint8_t cnt = timer_getInterval();
	outstanding_ticks += cnt;
	sched_add_interrupt(SCHED_TIMER_INT, soft_interrupt);


	if( num_realtime_clock > 0 ) {
		timer_set_hw_interval(	timer_update_realtime_clock(cnt) );
	}
}

