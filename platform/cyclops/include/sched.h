#ifndef SCHED_H
#define SCHED_H

uint8_t cyclops_direct_memory_access_status;
#define TOSH_CYCLOPS_IS_DIRECT_MEMORY_ACCESS cyclops_direct_memory_access_status==1
#define TOSH_CYCLOPS_SET_DIRECT_MEMORY_ACCESS()  cyclops_direct_memory_access_status=1
#define TOSH_CYCLOPS_RESET_DIRECT_MEMORY_ACCESS() cyclops_direct_memory_access_status=0


#endif
