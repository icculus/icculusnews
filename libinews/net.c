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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

/* don't like gotos? deal. I don't like duplicating code. */
#define FUNC_END(__success, __failure) { 																			 \
																				int __i; 															 \
																				end_success: 													 \
																					__i = 0; 														 \
																					__inews_errno = (__inews_errno ==    \
																													 ERR_STORYTOOLONG) ? \
																													 ERR_STORYTOOLONG :  \
																													 ERR_SUCCESS;				 \
																					goto real_end; 											 \
																				end_failure: 													 \
																					__i = 1;	 													 \
																				real_end: 														 \
																					pthread_mutex_trylock(&net_mutex);	 \
																					pthread_mutex_unlock(&net_mutex);		 \
																					return __i ? __failure : __success;	 \
								 											  }

Sint8 INEWS_connect(const char *hostname, Uint32 port) {
    struct hostent *hostents;

    pthread_mutex_lock(&net_mutex);

    serverstate.hostname = strdup(hostname);
    serverstate.port = port;

    hostents = gethostbyname(hostname);

    if (!hostents) {
        printf("Error returned from gethostbyname(): ");
        switch(h_errno) {
        case HOST_NOT_FOUND:
            printf("host not found.\n");
            break;
        case NO_ADDRESS:
            printf("no answer from DNS.\n");
            break;
        case NO_RECOVERY:
            printf("permanent failure.\n");
            break;
        case TRY_AGAIN:
            printf("temporary failure; try again.\n");
            break;
        default:
            printf("unknown error.\n");
        }

        __inews_errno = ERR_GENERIC;
        goto end_failure;
    }

    sa_len = sizeof(sa);
    memset(&sa, 0, sa_len);

    memcpy(&sa.sin_addr, hostents->h_addr_list[0], hostents->h_length);
    sa.sin_port = htons(port);
    sa.sin_family = AF_INET;

    if (!(fd = socket(PF_INET, SOCK_STREAM, 0))) {
        printf("Error returned from socket(): %s\n", strerror(errno));

        __inews_errno = ERR_GENERIC;
        goto end_failure;
    }

    if (connect(fd, (struct sockaddr *)&sa, sa_len) != 0) {
        printf("Error returned from connect(): %s\n", strerror(errno));

        __inews_errno = ERR_GENERIC;
        goto end_failure;
    }

    fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL) | O_NONBLOCK));

    {
        char welcome_msg[80];

        memset(welcome_msg, 0, 80);

        if (__read_line(welcome_msg, 80) == 0) {
            char *offset;

            if ((offset = strstr((char *)welcome_msg, "daemon")) != NULL)
                serverstate.verstring = strdup(offset + 7);
        } else {
            goto end_failure;
        }
    }

    serverstate.connected = TRUE;

    goto end_success;

    FUNC_END(0, -1)
}

Sint8 INEWS_auth(const char *username, const char *password) {
    char authstring[255];
    char respstring[81];

    if (!serverstate.connected) {
        __inews_errno = ERR_DISCONNECTED;
        goto end_failure;
    }

    memset(authstring, 0, 255);
    memset(respstring, 0, 81);

    pthread_mutex_lock(&net_mutex);

    if (username) {
        sprintf(authstring, "AUTH \"%s\" \"%s\"\n", username, password);
    } else {
        strcat(authstring, "AUTH -\n");
    }

    __write_block((char *)authstring);

    if (__read_line((char *)respstring, 80) < 0) {
        goto end_failure;
    }

    switch(respstring[0]) {
    case '+':
        serverstate.username = username ? strdup(username) : strdup("anonymous");
        serverstate.password = password ? strdup(password) : NULL;
        serverstate.uid = atoi(respstring + 2);
        serverstate.qid = atoi(strchr(respstring, ',') + 1);
        keep_nopping = 1;
        pthread_create(&nop_thread, NULL, __nop_thread, NULL);
        goto end_success;
        break;
    default:
        switch(respstring[2]) {
        case 'c': /* "can't execute query" */
            INEWS_disconnect();
            __inews_errno = ERR_SERVFAIL;
            break;
        case 'A': /* "Authorization for <foo> failed." */
            __inews_errno = ERR_NOSUCHUSER;
            break;
        case 'T': /* "This account has been disabled." */
            __inews_errno = ERR_UNAUTHORIZED;
            break;
        case 'F': /* "Failed to set default queue" */
            __inews_errno = ERR_NOSUCHQUEUE;
            break;
        default:
            __print_protocol_fuckery_message();
            __inews_errno = ERR_GENERIC;
            break;
        }
        goto end_failure;
    }

    FUNC_END(0, -1)
}

Sint8 INEWS_retrQueueInfo() {
    QueueInfo *temp_ptr;
    char temp_data[256];
    IList *temp_iter;
    Uint8 record_pos = 0;

    if (!serverstate.connected) {
        __inews_errno = ERR_DISCONNECTED;
        goto end_failure;
    }

    pthread_mutex_lock(&net_mutex);

    __write_block("ENUM queues\n");
    if (__read_line(temp_data, 255) < 0) { /* to get rid of "+ Here comes..." */
        goto end_failure;
    }

    while (1) {
        memset(temp_data, 0, 256);
        if (__read_line(temp_data, 255) < 0) {
            goto end_failure;
        }

        if (!strcmp(temp_data, ".")) break;

        if (!record_pos) {
            temp_ptr = (QueueInfo *)malloc(sizeof(QueueInfo));
            temp_ptr->qid = atoi(temp_data);
            record_pos = 1;
        } else {
            temp_ptr->name = strdup(temp_data);
            qinfoptr = ilist_append_data(qinfoptr, temp_ptr);
            record_pos = 0;
        }
    }

    temp_iter = qinfoptr;

    while (temp_iter) {
        QueueInfo *temp_qinfo = (QueueInfo *)temp_iter->data;
        memset(temp_data, 0, 256);

        sprintf(temp_data, "QUEUEINFO %i\n", temp_qinfo->qid);

        __write_block(temp_data);

        if (__read_line(temp_data, 255) < 0) {
            goto end_failure;
        }

        record_pos = 0;

        while (record_pos < 9) {
            memset(temp_data, 0, 256);

            if (__read_line(temp_data, 255) < 0) {
                goto end_failure;
            }

            switch (++record_pos) {
            case 2: temp_qinfo->description = strdup(temp_data); break;
            case 3: temp_qinfo->digest = strdup(temp_data); break;
            case 4: temp_qinfo->singleitem = strdup(temp_data); break;
            case 5: temp_qinfo->home = strdup(temp_data); break;
            case 6: temp_qinfo->rdf = strdup(temp_data); break;
            case 7: strptime(temp_data, "%Y-%m-%d %T", &(temp_qinfo->ctime)); break;
            case 8: temp_qinfo->owneruid = atoi(temp_data); break;
            case 9: temp_qinfo->ownername = strdup(temp_data); break;
            }
        }

        temp_iter = ilist_next(temp_iter);
    }

    goto end_success;

    FUNC_END(0, -1)
}

Sint8 INEWS_changeQueue(int qid) {
    char tempstring[255];

    if (!serverstate.connected) {
	__inews_errno = ERR_DISCONNECTED;
	goto end_failure;
    }

    if (qid == serverstate.qid) {
	goto end_success;
    }

    memset(tempstring, 0, 255);

    pthread_mutex_lock(&net_mutex);

    sprintf(tempstring, "QUEUE %i\n", qid);

    __write_block(tempstring);

    memset(tempstring, 0, 255);

    if (__read_line(tempstring, 254) < 0) {
	goto end_failure;
    }

    switch(tempstring[0]) {
    case '+':
	serverstate.qid = qid;
	goto end_success;
	break;
    case '-':
	switch (tempstring[2]) {
	case 'c': /* "can't execute query" */
	    INEWS_disconnect();
	    __inews_errno = ERR_SERVFAIL;
	    break;
	case 'C': /* "Can't select that queue" */
	    __inews_errno = ERR_NOSUCHQUEUE;
	    break;
	default:
	    __print_protocol_fuckery_message();
	    __inews_errno = ERR_GENERIC;
	    break;
	}
	goto end_failure;
    }

    FUNC_END(0, 1)
}

ArticleInfo **INEWS_digest(int offset, int n) {
    char tempstring[256], off_str[32];
    ArticleInfo **retval;
    ArticleInfo *tempinfo, *cacheptr;
    int record_pos = 0, count = 0;
    bool eor = FALSE;
    int requested_offset = offset;

    if (!serverstate.connected) {
	__inews_errno = ERR_DISCONNECTED;
	goto end_failure;
    }

    retval = (ArticleInfo **)malloc(n * sizeof(ArticleInfo *));
    memset(retval, 0, sizeof(retval));

    /* iterate through the cache, and set the offset to the lowest-numbered
     * cached item. assume that the items are listed in descending order of
     * article ID, and that there are no gaps in the cache. */

    for (IList *ptr = digestcache; ptr; ptr = ilist_next(ptr)) {
        ArticleInfo *tempptr;
        ArticleLinkedListHeader *curdata = ((ArticleLinkedListHeader *)(ptr->data));
        if (curdata->qid == INEWS_getQID())
            for (IList *aptr = curdata->head; aptr && (count < n); aptr = ilist_next(aptr)) {
                if (requested_offset) {
                    if (((ArticleInfo *)(aptr->data))->aid < requested_offset) {
                        tempptr = __copy_articleinfo((ArticleInfo *)aptr->data);
                        retval[count++] = tempptr;
                        if (((ArticleInfo *)(aptr->data))->aid < offset) {
                            offset = ((ArticleInfo *)(aptr->data))->aid;
                        }
                    }
                } else {
                    tempptr = __copy_articleinfo((ArticleInfo *)aptr->data);
                    retval[count++] = tempptr;
                    offset = ((ArticleInfo *)(aptr->data))->aid;
                }
            }
    }

    pthread_mutex_lock(&net_mutex);

    /* convert the offset to a string */
    if (offset > 0)
        sprintf(off_str, "%i", offset);
    else
        sprintf(off_str, "-");

    sprintf(tempstring, "DIGEST %s %i\n", off_str, n);

    __write_block(tempstring);

    if (__read_line(tempstring, 255) < 0) { /* to get rid of "+ Here comes..." */
	goto end_failure;
    }

    while (1) {
        tempinfo = (ArticleInfo *)malloc(sizeof(ArticleInfo));
        cacheptr = (ArticleInfo *)malloc(sizeof(ArticleInfo));

	record_pos = 0;

	while (record_pos < 8) {
	    memset(tempstring, 0, 256);

	    if (__read_line(tempstring, 255) < 0) {
		goto end_failure;
	    }

	    if (!strcmp(tempstring, ".")) {
		eor = TRUE;
		break;
	    }

            /* we have to make a full copy for the cache, so that we're not
             * dependent on the existence of the temporary copy */

	    switch (++record_pos) {
            case 1:
                tempinfo->aid = atoi(tempstring);
                cacheptr->aid = tempinfo->aid; break;
            case 2:
                tempinfo->title = strdup(tempstring);
                cacheptr->title = strdup(tempstring); break;
            case 3:
                strptime(tempstring, "%Y-%m-%d %T", &(tempinfo->ctime));
                memcpy(&(cacheptr->ctime), &(tempinfo->ctime), sizeof(struct tm)); break;
            case 4:
                tempinfo->owneruid = atoi(tempstring);
                cacheptr->owneruid = tempinfo->owneruid; break;
            case 5:
                tempinfo->ownername = strdup(tempstring);
                cacheptr->ownername = strdup(tempstring); break;
            case 6:
                tempinfo->dottedip = strdup(tempstring);
                cacheptr->dottedip = strdup(tempstring); break;
            case 7:
                tempinfo->approved = atoi(tempstring);
                cacheptr->approved = tempinfo->approved; break;
            case 8:
                tempinfo->deleted = atoi(tempstring);
                cacheptr->deleted = tempinfo->deleted; break;
	    }
	}

	if (eor) break;

        retval[count++] = tempinfo;

        /* cache the new digest */
        {
            IList *ptr;
            ArticleLinkedListHeader *thisqueue;

	    for (ptr = digestcache; ptr; ptr = ilist_next(ptr)) {
                if (((ArticleLinkedListHeader *)(ptr->data))->qid == INEWS_getQID()) break;
                thisqueue = ((ArticleLinkedListHeader *)(ptr->data));
            }

	    if (!ptr) {
                thisqueue = (ArticleLinkedListHeader *)malloc(sizeof(ArticleLinkedListHeader));
                thisqueue->qid = INEWS_getQID();
                thisqueue->head = NULL;
                digestcache = ilist_append_data(digestcache, thisqueue);
            }

            thisqueue->head = ilist_append_data(thisqueue->head, cacheptr);
        }
    }

    /* if we hit a premature end-of-record, then we'll need to shrink down our
     * return to prevent problems when we try to free it */
    retval = (ArticleInfo **)realloc(retval, (count * sizeof(ArticleInfo *)));
    retval[count] = NULL;

    goto end_success;

    FUNC_END(retval, NULL)
}

Sint8 INEWS_submitArticle(char *title, char *body) {
    return INEWS_submitEditArticle(title, body, -1);
}

Sint8 INEWS_submitEditArticle(char *title, char *body, int aid) {
    char tempstring[512];
    Uint32 maxlen;

    __inews_errno = ERR_SUCCESS;

    if (!title || !strcmp(title, "")) {
	__inews_errno = ERR_GENERIC;
	return -1;
    } else if (!serverstate.qid) {
	__inews_errno = ERR_NOSUCHQUEUE;
	return -1;
    }

    memset(tempstring, 0, 512);

    if (aid > 0) {
	sprintf(tempstring, "EDIT %i %s\n", aid, title);
    } else {
	sprintf(tempstring, "POST %s\n", title);
    }

    pthread_mutex_lock(&net_mutex);

    __write_block(tempstring);

    memset(tempstring, 0, 512);

    if (__read_line(tempstring, 511) < 0) {
	goto end_failure;
    }

    switch (tempstring[0]) {
    case '+':
	/* as of glibc 2.2.5 trunk, the sscanf() line segfaults.  gg, glibc. */
#ifndef I_LIKE_BROKEN_SSCANF_AND_I_CANNOT_LIE
	sscanf(tempstring, "+ You've got %ui bytes; Go, hose.", &maxlen);
#else
	maxlen = strtol(strstr(tempstring, "got") + 4, NULL, 10);
#endif
	break;
    default:
	__print_protocol_fuckery_message();
	__inews_errno = ERR_GENERIC;
	goto end_failure;
    }

    if (strlen(body) > maxlen) {
	__inews_errno = ERR_STORYTOOLONG;
	memset(body + maxlen, 0, strlen(body + maxlen));
    }

    __write_block(body);
    __write_block("\n.\n");

    memset(tempstring, 0, 512);

    if (__read_line(tempstring, 511) < 0) {
	goto end_failure;
    }

    switch (tempstring[0]) {
    case '+': break;
    case '-':
	switch (tempstring[2]) {
	case 'c': /* can't execute query */
	    __inews_errno = ERR_SERVFAIL;
	    INEWS_disconnect();
	    break;
	default:
	    __print_protocol_fuckery_message();
	    __inews_errno = ERR_GENERIC;
	}
	goto end_failure;
	break;
    default:
	__print_protocol_fuckery_message();
	goto end_failure;
    }

    goto end_success;

    FUNC_END(0, -1);
}

Sint8 INEWS_changeApprovalStatus(Uint32 aid, bool approve) {
    char tempstring[256];
    ArticleInfo **artinfo;

    if (!serverstate.connected) {
	__inews_errno = ERR_DISCONNECTED;
	goto end_failure;
    }

    if (!serverstate.qid) {
	__inews_errno = ERR_NOSUCHQUEUE;
	goto end_failure;
    }

    artinfo = INEWS_digest(aid + 1, 1);

    if (artinfo[0]->approved == approve) {
        INEWS_freeDigest(artinfo);
        goto end_success;
    } else if (artinfo[0]->deleted) {
        INEWS_freeDigest(artinfo);
        __inews_errno = ERR_GENERIC;
        goto end_failure;
    } else INEWS_freeDigest(artinfo);


    memset(tempstring, 0, 256);

    if (approve) {
	sprintf(tempstring, "APPROVE %i\n", aid);
    } else {
	sprintf(tempstring, "UNAPPROVE %i\n", aid);
    }

    pthread_mutex_lock(&net_mutex);

    __write_block(tempstring);

    memset(tempstring, 0, 256);

    if (__read_line(tempstring, 255) < 0) {
	goto end_failure;
    }

    switch(tempstring[0]) {
    case '+':
	goto end_success;
        __get_article_cache_ptr(INEWS_getQID(), aid)->approved = approve;
	break;
    case '-':
	switch (tempstring[2]) {
	case 'c': /* "can't execute query" */
	    __inews_errno = ERR_SERVFAIL;
	    INEWS_disconnect();
	    break;
	case 'Y': /* "You don't have permission" */
	    __inews_errno = ERR_UNAUTHORIZED;
	    break;
	case 'F': /* "Failed to toggle approval flag" */
	    __inews_errno = ERR_GENERIC;
	    break;
	default:
	    __print_protocol_fuckery_message();
	    __inews_errno = ERR_GENERIC;
	}
	goto end_failure;
	break;
    default:
	__print_protocol_fuckery_message();
	__inews_errno = ERR_GENERIC;
	goto end_failure;
    }

    FUNC_END(0, -1);
}

Sint8 INEWS_changeDeletionStatus(Uint32 aid, bool deleteflag) {
    char tempstring[256];
    ArticleInfo **artinfo;

    if (!serverstate.connected) {
	__inews_errno = ERR_DISCONNECTED;
	goto end_failure;
    }

    if (!serverstate.qid) {
	__inews_errno = ERR_NOSUCHQUEUE;
	goto end_failure;
    }

    artinfo = INEWS_digest(aid + 1, 1);

    if (artinfo[0]->deleted == deleteflag) {
        INEWS_freeDigest(artinfo);
        goto end_success;
    } else INEWS_freeDigest(artinfo);

    memset(tempstring, 0, 256);

    if (deleteflag) {
	sprintf(tempstring, "DELETE %i\n", aid);
    } else {
	sprintf(tempstring, "UNDELETE %i\n", aid);
    }

    pthread_mutex_lock(&net_mutex);

    __write_block(tempstring);

    memset(tempstring, 0, 256);

    if (__read_line(tempstring, 255) < 0) {
	goto end_failure;
    }

    switch(tempstring[0]) {
    case '+':
	goto end_success;
	__get_article_cache_ptr(INEWS_getQID(), aid)->deleted = deleteflag;
	break;
    case '-':
	switch (tempstring[2]) {
	case 'c': /* "can't execute query" */
	    __inews_errno = ERR_SERVFAIL;
	    INEWS_disconnect();
	    break;
	case 'Y': /* "You don't have permission" */
	    __inews_errno = ERR_UNAUTHORIZED;
	    break;
	case 'F': /* "Failed to toggle deletion flag" */
	    __inews_errno = ERR_GENERIC;
	    break;
	default:
	    __print_protocol_fuckery_message();
	    __inews_errno = ERR_GENERIC;
	}
	goto end_failure;
	break;
    default:
	__print_protocol_fuckery_message();
	__inews_errno = ERR_GENERIC;
	goto end_failure;
    }

    FUNC_END(0, -1);
}

void INEWS_disconnect() {
    IList *qinfo_iter = qinfoptr;

    if (!serverstate.connected) return;

    keep_nopping = 0;
    pthread_join(nop_thread, NULL);
    __write_block("QUIT\n");
    close(fd);
    serverstate.connected = FALSE;

    /* clean up dynamically-allocated crap. */
    free(serverstate.hostname);
    free(serverstate.username);
    free(serverstate.password);
    free(serverstate.verstring);

    if (!qinfo_iter) return;

    while (1) {
	__free_queue_info_list_element(qinfo_iter);
	qinfo_iter = ilist_remove(qinfo_iter);
	if (!qinfo_iter) break;
    }
}

/* internal: returns one line of text from the server,
 * /sans/ the newline. */

Sint8 __read_line(char *str, int max_sz) {
    ssize_t count = 0, len = 0;

    while (len < max_sz) {
	count = read(fd, (str + len), 1);
	if (count < 0 && (errno != EAGAIN && errno != EINTR)) {
	    printf("Error from read(): %s\n", strerror(errno));
	    __inews_errno = ERR_GENERIC;
	    return ERR_GENERIC;
	} else if (count == 0) {
	    printf("Connection reset by peer\n");
	    INEWS_disconnect();
	    __inews_errno = ERR_DISCONNECTED;
	    return ERR_DISCONNECTED;
	} else if (count > 0) {
	    len += count;
	    if (*(str + len - 1) == '\n') break;
	}
    }

    __chop(str);

    return ERR_SUCCESS;
}

Sint8 __write_block(char *str) {
    ssize_t count = 0, len = 0;

    while (len < (ssize_t)strlen(str)) {
	count = write(fd, (str + len), (strlen(str) - len));
	if (count < 0 && (errno != EAGAIN && errno != EINTR)) {
	    printf("Error from write(): %s\n", strerror(errno));
	    return ERR_GENERIC;
	} else {
	    len += count;
	}
    }

    return ERR_SUCCESS;
}

void *__nop_thread(void *foo) {
    char throwaway[81];

    foo = foo; /* because I can. and because gcc -Wall -W -pedantic is a bitch. */

    while (keep_nopping) {
	sleep(2);
	pthread_mutex_lock(&net_mutex);

	__write_block("NOOP\n");
	if (__read_line(throwaway, 80)) {
	    pthread_mutex_unlock(&net_mutex);
	    return NULL;
	}

	pthread_mutex_unlock(&net_mutex);
    }

    return NULL;
}
