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

#include "internals.h"
#include "IcculusNews.h"

const INEWS_Version *INEWS_getVersion() {
    __inews_version.major = INEWS_MAJOR;
    __inews_version.minor = INEWS_MINOR;
    __inews_version.rev = INEWS_REV;
    return &__inews_version;
}

inline const char *INEWS_getServerVersion() {
    return serverstate.connected ? serverstate.verstring : NULL;
}

inline const char *INEWS_getHost() {
    return serverstate.connected ? serverstate.hostname : NULL;
}

inline Uint16 INEWS_getPort() {
    return serverstate.connected ? serverstate.port : 0;
}

inline const char *INEWS_getUserName() {
    return serverstate.connected ? serverstate.username : NULL;
}

inline Uint16 INEWS_getUID() {
    return serverstate.connected ? serverstate.uid : 0;
}

inline Uint16 INEWS_getQID() {
    return serverstate.connected ? serverstate.qid : 0;
}

QueueInfo *INEWS_getQueueInfo(int qid) {
    IList *iter = qinfoptr;
				
    do {
	if (((QueueInfo *)(iter->data))->qid == qid)
	    return ((QueueInfo *)(iter->data));
    } while ((iter = ilist_next(iter)));

    return NULL;
}

QueueInfo **INEWS_getAllQueuesInfo() {
    QueueInfo **retval;
    IList *iter = qinfoptr;

    retval = (QueueInfo **)malloc(ilist_length(qinfoptr) * sizeof(QueueInfo *));

    for (Uint32 i = 0; i < ilist_length(qinfoptr); i++, iter = ilist_next(iter)) {
	QueueInfo *temp;

	temp = (QueueInfo *)malloc(sizeof(QueueInfo));

	memcpy(temp, iter->data, sizeof(QueueInfo));

	retval[i] = temp;
    }

    return retval;
}

void INEWS_freeDigest(ArticleInfo **digest) {
    for (Uint32 i = 0; i < (sizeof(digest) / sizeof(ArticleInfo *)); i++) {
	free(digest[i]->title);
	free(digest[i]->ownername);
	free(digest[i]->dottedip);
	free(digest[i]);
    }

    free(digest);
}

void INEWS_freeQueuesInfo(QueueInfo **qinfo) {
    for (Uint32 i = 0; i < (sizeof(qinfo) / sizeof(QueueInfo *)); i++) {
	free(qinfo[i]->name);
	free(qinfo[i]->description);
	free(qinfo[i]->digest);
	free(qinfo[i]->singleitem);
	free(qinfo[i]->home);
	free(qinfo[i]->rdf);
	free(qinfo[i]->ownername);
	free(qinfo[i]);
    }

    free(qinfo);
}

inline Sint8 INEWS_getLastError() {
    return __inews_errno;
}

void __free_queue_info_list_element(IList *ptr) {
    free(((QueueInfo *)(ptr->data))->name);
    free(((QueueInfo *)(ptr->data))->description);
    free(((QueueInfo *)(ptr->data))->digest);
    free(((QueueInfo *)(ptr->data))->singleitem);
    free(((QueueInfo *)(ptr->data))->home);
    free(((QueueInfo *)(ptr->data))->rdf);
    free(((QueueInfo *)(ptr->data))->ownername);
    free(ptr->data);
}

char *__chop(char *str) {
    char *temp = str;

    do { if (!isspace(*temp)) break; } while (temp++);

    memmove(str, temp, strlen(temp)+1);

    temp = str + strlen(str);

    while (--temp) { if (!isspace(*temp)) break; }

    *(temp + 1) = '\0';

    return str;
}

void __print_protocol_fuckery_message() {
    void *last_call[2];
    char **last_call_name;

    backtrace(last_call, 2);
    last_call_name = (char **)backtrace_symbols(last_call, 2);

    printf("Guru meditation error in %s: unexpected server response (try French?)\n",
	   last_call_name[1]);

    free(last_call_name);
}

ArticleInfo *__get_article_cache_ptr(Uint32 qid, Uint32 aid) {
    for (IList *qptr = digestcache; qptr; qptr = ilist_next(qptr)) {
	if (((ArticleLinkedListHeader *)(qptr->data))->qid == qid) {
	    for (IList *aptr = ((ArticleLinkedListHeader *)(qptr->data))->head;
		 aptr; aptr = ilist_next(aptr)) {
		if (((ArticleInfo *)(aptr->data))->aid == aid)
                    return (ArticleInfo *)(aptr->data);
	    }
	}
    }
    return NULL;
}

ArticleInfo *__copy_articleinfo(ArticleInfo *ptr) {
    ArticleInfo *retval = (ArticleInfo *)malloc(sizeof(ArticleInfo));

    memcpy(retval, ptr, sizeof(ArticleInfo));

    retval->title = strdup(ptr->title);
    retval->ownername = strdup(ptr->ownername);
    retval->dottedip = strdup(ptr->dottedip);

    return retval;
}
