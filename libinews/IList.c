/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#include "IList.h"

/* IList, the helping phriendly doubly-linked list. */

IList *ilist_append(IList *list, IList *new);
IList *ilist_append_data(IList *list, void *data);

IList *ilist_prepend(IList *list, IList *new);
IList *ilist_prepend_data(IList *list, void *data);

IList *ilist_remove(IList *ptr);

unsigned int ilist_length(IList *ptr);

IList *__get_last(IList *list);
IList *__get_first(IList *list);

/* FIXME: extremely quick implementation. probably 50 zillion bugs in the next
 * 65 lines, but who's counting? -- vogon. */

IList *ilist_append(IList *list, IList *new) {
	new->prev = __get_last(list);
	if (new->prev) new->prev->next = new;
	new->next = NULL;

	return __get_first(new);
}

IList *ilist_append_data(IList *list, void *data) {
	IList *new = (IList *)malloc(sizeof(IList));

	new->data = data;

	return ilist_append(list, new);
}

IList *ilist_prepend(IList *list, IList *new) {
	new->next = __get_first(list);
	if (new->next) new->next->prev = new;
	new->prev = NULL;

	return new;
}

IList *ilist_prepend_data(IList *list, void *data) {
	IList *new = (IList *)malloc(sizeof(IList));

	new->data = data;

	return ilist_prepend(list, new);
}

IList *ilist_remove(IList *ptr) {
	if (ptr->prev) ptr->prev->next = ptr->next;
	if (ptr->next) ptr->next->prev = ptr->prev;

	return __get_first(ptr->next);
}

unsigned int ilist_length(IList *ptr) {
	int count = 0;
				
	for (IList *iter = __get_first(ptr); iter != NULL; iter = iter->next) {
		count++;
	}

	return count;
}

IList *__get_last(IList *list) {
	IList *last = list;

	while (last && last->next) { last = last->next; }

	return last;
}

IList *__get_first(IList *list) {
	IList *first = list;

	while (first && first->prev) { first = first->prev; }

	return first;
}
