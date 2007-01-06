#include <sos_list.h>
#include <sos_sched.h>

void list_insert_before(list_link_t* before, list_link_t* toInsert)
{
	toInsert->l_next = before;
	toInsert->l_prev = before->l_prev;
	before->l_prev->l_next = toInsert;
	before->l_prev = toInsert;
}

void list_insert_head(list_t* list, list_link_t* element)
{
	list_insert_before(list->l_next, element);
}

void list_insert_tail(list_t* list, list_link_t* element)
{
	list_insert_before(list, element);
}

void list_remove(list_link_t* ll)
{
	list_link_t *before = ll->l_prev;
	list_link_t *after = ll->l_next;
	if (before->l_next != ll && after->l_prev != ll)
	{
		ll->l_next = 0;
		ll->l_prev = 0;
		return;
	} else if (before->l_next != ll || after->l_prev != ll)
	{
		DEBUG("ERROR: corrupted queue\n");
		return;
	}
	before->l_next = after;
	after->l_prev = before;
	ll->l_next = NULL;
	ll->l_prev = NULL;
}



void list_remove_head(list_t* list)
{
	list_remove((list)->l_next);
}



void list_remove_tail(list_t* list)
{
	list_remove((list)->l_prev);
}

void list_init(list_t* list)
{
	list->l_next = list->l_prev = list;
}

bool list_empty(list_t* list)
{
	return ((list->l_next == list)? true:false);
}
