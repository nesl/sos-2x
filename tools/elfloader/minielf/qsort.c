/**
 * \file qsort.c
 * \brief Quick Sort of a linked list
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <linklist.h>

//#define DEBUG(arg...) printf(arg)
#define DEBUG(arg...)

static list_t* partition(list_t* lb, list_t* rb, 
			 ltcmp_func_t ltcmp, swap_func_t swapfunc);


void qsortlist(list_t* lb, list_t* rb, ltcmp_func_t ltcmp, swap_func_t swapfunc)
{
  list_t* m;
  DEBUG("Starting QuickSort\n");
  if (lb == rb) return;                                        // Single element list
  m = partition(lb, rb, ltcmp, swapfunc);               // Partition list around a pivot
  if (m != lb) qsortlist(lb, m, ltcmp, swapfunc);       // Sort left side
  if (m != rb) qsortlist(m->next, rb, ltcmp, swapfunc); // Sort right side
  return;
}

static list_t* partition(list_t* lb, list_t* rb, 
			 ltcmp_func_t ltcmp, swap_func_t swapfunc)
{
  list_t *i, *j, *pivot;
  int done;
  DEBUG("Starting partition\n");
  done = 0;
  pivot = lb;

  i = lb; j = rb;
  
  while(1){
    // Find first element from right that is strictly less than pivot
    while (ltcmp(j, pivot) == 0){
      if (1 == done)
	break;
      j = j->prev;
      if (i == j) {
	done = 1;
      }
    }
    if (done) return j;

    // Find first element from left that is greater than or equal to pivot
    while (ltcmp(i, pivot)){
      if (1 == done)
	break;
      i = i->next;
      if (i == j){
	done = 1;
      }
    }
    if (done) return j;

    // Swap - Need to track the reference to the pivot element
    if (pivot == i)
      pivot = j;
    swapfunc(i, j);

    j = j->prev;
    if (i == j) done = 1;
    i = i->next;
    if (i == j) done = 1;

  }
  return NULL;
}
