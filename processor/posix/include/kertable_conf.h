
#ifndef _KERTABLE_CONF_H
#define _KERTABLE_CONF_H

extern void* ker_jumptable[128];
#define get_kertable_entry(k)  ker_jumptable[(k)]

#endif

