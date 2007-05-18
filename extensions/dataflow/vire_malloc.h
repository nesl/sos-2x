#ifndef _VIRE_MALLOC_H_
#define _VIRE_MALLOC_H_

// To implement a separate memory manager for ViRe,
// define the functions below appropriately.
// Currently, default memory allocator is used.
#define vire_malloc(size, pid) (ker_malloc(size, pid))
#define vire_free(ptr, size) (ker_free(ptr))

#ifdef USE_VIRE_TOKEN_MEM
// Use custom memory allocator for tokens if this
// optimization is enabled at compile time.
void *token_malloc(sos_pid_t pid);
void token_free(void *ptr);
#else
// Use default memory allocator for tokens also.
#define token_malloc(pid) (ker_malloc(sizeof(token_type_t), pid))
#define token_free(ptr) (ker_free(ptr))
#endif

#endif

