/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 * 
 * [ -- Insert GPL boilerplate here -- ]
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
	/*GList *iter = qinfoptr;*/
	IList *iter = qinfoptr;
				
	do {
		if (((QueueInfo *)(iter->data))->qid == qid) 
			return ((QueueInfo *)(iter->data));
	} while ((iter = /*g_list_next(iter)*/ ilist_next(iter)));

	return NULL;
}

QueueInfo **INEWS_getAllQueuesInfo() {
	QueueInfo **retval;
	/*GList*/ IList *iter = qinfoptr;

	retval = (QueueInfo **)malloc(/*g_list_length*/ilist_length(qinfoptr) * sizeof(QueueInfo *));
	
	for (Uint32 i = 0; i < /*g_list_length*/ilist_length(qinfoptr); i++, iter = /*g_list_next*/ilist_next(iter)) {
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

void __free_queue_info_list_element(/*GList *ptr*/ IList *ptr) {
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

void __print_protocol_fuckery_message() {
  void *last_call[2];
  char **last_call_name;

  backtrace(last_call, 2);
  last_call_name = (char **)backtrace_symbols(last_call, 2);

  printf("Guru meditation error in %s: unexpected server response (try French?)\n",
         last_call_name[1]);

  free(last_call_name);
}
