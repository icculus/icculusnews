/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 * 
 * [ -- Insert GPL boilerplate here -- ]
 * 
 */

#include "internals.h"
#include "IcculusNews.h"

INEWS_Version *INEWS_getVersion() {
	__inews_version.major = INEWS_MAJOR; 
	__inews_version.minor = INEWS_MINOR; 
	__inews_version.rev = INEWS_REV;
	return &__inews_version;
}

char *INEWS_getServerVersion() {
	return serverstate.verstring;				
}

char *INEWS_getHost() {
	return serverstate.hostname;
}

int INEWS_getPort() {
	return serverstate.port;
}

const char *INEWS_getUserName() {
	return serverstate.username;
}

int INEWS_getUID() {
	return serverstate.uid;
}

int INEWS_getQID() {
	return serverstate.qid;
}

QueueInfo *INEWS_getQueueInfo(int qid) {
	GList *iter = qinfoptr;
				
	do {
		if (((QueueInfo *)(iter->data))->qid == qid) 
			return ((QueueInfo *)(iter->data));
	} while (iter = g_list_next(iter));
}

QueueInfo **INEWS_getAllQueuesInfo() {
	QueueInfo **retval;
	int i;
	GList *iter = qinfoptr;

	retval = (QueueInfo **)malloc(g_list_length(qinfoptr) * sizeof(QueueInfo *));
	
	for (i = 0; i < g_list_length(qinfoptr); i++, iter = g_list_next(iter)) {
		QueueInfo *temp;
		
		temp = (QueueInfo *)malloc(sizeof(QueueInfo));
	
		memcpy(temp, iter->data, sizeof(QueueInfo));

		retval[i] = temp;
	}

	return retval;
}

void INEWS_freeDigest(ArticleInfo **digest) {
	int i;

	for (i = 0; i < (sizeof(digest) / sizeof(ArticleInfo *)); i++) {
		free(digest[i]->title);
		free(digest[i]->ownername);
		free(digest[i]->dottedip);
		free(digest[i]);
	}

	free(digest);
}

void INEWS_freeQueuesInfo(QueueInfo **qinfo) {
	int i;

	for (i = 0; i < (sizeof(qinfo) / sizeof(QueueInfo *)); i++) {
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

int INEWS_getLastError() {
	return __inews_errno;
}

void __free_queue_info_list_element(GList *ptr) {
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
	return g_strstrip(str);
}