/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
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

extern IList *ilist_append(IList *list, IList *new);
extern IList *ilist_append_data(IList *list, void *data);

extern IList *ilist_prepend(IList *list, IList *new);
extern IList *ilist_prepend_data(IList *list, void *data);

extern IList *ilist_remove(IList *ptr);

extern unsigned int ilist_length(IList *ptr);

#define ilist_free(ptr) free(ptr)
#define ilist_prev(ptr) ptr->prev
#define ilist_next(ptr) ptr->next
