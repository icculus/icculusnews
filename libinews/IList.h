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

/* IList, the helping phriendly doubly-linked list. */

#include <stdlib.h>

struct _IList {
	void *data;
	struct _IList *prev;
	struct _IList *next;
};

typedef struct _IList IList;

extern IList *ilist_append(IList *list, IList *new_ptr);
extern IList *ilist_append_data(IList *list, void *data);

extern IList *ilist_prepend(IList *list, IList *new_ptr);
extern IList *ilist_prepend_data(IList *list, void *data);

extern IList *ilist_insert(IList *insert_after, IList *new_ptr);
extern IList *ilist_insert_data(IList *insert_after, void *data);

extern IList *ilist_remove(IList *ptr);

extern IList *ilist_first(IList *ptr);
extern IList *ilist_last(IList *ptr);

extern unsigned int ilist_length(IList *ptr);

#define ilist_free(ptr) free(ptr)
#define ilist_prev(ptr) ptr->prev
#define ilist_next(ptr) ptr->next
