/**
 * \file linklist.h
 * \brief Generic header for common link list operations
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _LINKLISTS_H_
#define _LINKLISTS_H_


// List Element
typedef struct _list_str {
  struct _list_str* next;
  struct _list_str* prev;
} list_t;

// Less-than Comparison Function Type
typedef int (*ltcmp_func_t)(list_t* lhop, list_t* rhop);

// Swap Function Type
typedef void (*swap_func_t)(list_t* a, list_t* b);


// Quick Sort Function
void qsortlist(list_t* lb, list_t* rb, 
	       ltcmp_func_t ltcmp, swap_func_t swapfunc);






#endif//_LINKLISTS_H_
