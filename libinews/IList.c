/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "IList.h"

/* IList, the helping phriendly doubly-linked list. */

IList *ilist_append(IList *list, IList *new_ptr);
IList *ilist_append_data(IList *list, void *data);

IList *ilist_prepend(IList *list, IList *new_ptr);
IList *ilist_prepend_data(IList *list, void *data);

IList *ilist_remove(IList *ptr);

IList *ilist_first(IList *list);
IList *ilist_last(IList *list);

unsigned int ilist_length(IList *ptr);

IList *__get_last(IList *list);
IList *__get_first(IList *list);

/* FIXME: extremely quick implementation. probably 50 zillion bugs in the next
 * 65 lines, but who's counting? -- vogon. */

IList *ilist_append(IList *list, IList *new_ptr) {
	new_ptr->prev = __get_last(list);
	if (new_ptr->prev) new_ptr->prev->next = new_ptr;
	new_ptr->next = NULL;

	return __get_first(new_ptr);
}

IList *ilist_append_data(IList *list, void *data) {
	IList *new_ptr = (IList *)malloc(sizeof(IList));

	new_ptr->data = data;

	return ilist_append(list, new_ptr);
}

IList *ilist_prepend(IList *list, IList *new_ptr) {
	new_ptr->next = __get_first(list);
	if (new_ptr->next) new_ptr->next->prev = new_ptr;
	new_ptr->prev = NULL;

	return new_ptr;
}

IList *ilist_prepend_data(IList *list, void *data) {
	IList *new_ptr = (IList *)malloc(sizeof(IList));

	new_ptr->data = data;

	return ilist_prepend(list, new_ptr);
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

IList *ilist_first(IList *list) { return __get_first(list); }
IList *ilist_last(IList *list) { return __get_last(list); }
