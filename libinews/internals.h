/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 * 
 * [ -- Insert GPL boilerplate here -- ]
 * 
 */

#ifndef __HAVE_INTERNALS_H
#define __HAVE_INTERNALS_H

#include "IcculusNews.h"

#include <netinet/in.h>
#include <pthread.h>

/* squelch all the implicit function decl warnings */
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <errno.h>
#include <ctype.h>

#include "IList.h"

#define __USE_BSD
#include <string.h>

#define INEWS_MAJOR __INEWS_LINKTIME_MAJOR
#define INEWS_MINOR __INEWS_LINKTIME_MINOR
#define INEWS_REV __INEWS_LINKTIME_REV

INEWS_Version __inews_version;

typedef struct {
	bool connected;
	char *hostname;
	Uint16 port;
	char *username;
	char *password;
	Uint8 uid;
	Uint8 qid;
	char *verstring;
} ServerState;

ServerState serverstate;
IList *qinfoptr;
int fd;
struct sockaddr_in sa;
size_t sa_len;
int keep_nopping;
int __inews_errno;

pthread_mutex_t net_mutex;
pthread_t nop_thread;

Sint8 INEWS_init();
void INEWS_deinit();

const INEWS_Version *INEWS_getVersion();
inline const char *INEWS_getServerVersion();
inline const char *INEWS_getHost();
inline Uint16 INEWS_getPort();
inline const char *INEWS_getUserName();
inline Uint16 INEWS_getUID();
inline Uint16 INEWS_getQID();
QueueInfo *INEWS_getQueueInfo(int qid);
QueueInfo **INEWS_getAllQueuesInfo();
inline Sint8 INEWS_getLastError();

void INEWS_freeDigest(ArticleInfo **digest);
void INEWS_freeQueuesInfo(QueueInfo **qinfo);

Sint8 INEWS_connect(const char *hostname, Uint32 port);
Sint8 INEWS_auth(const char *username, const char *password);
Sint8 INEWS_retrQueueInfo();
Sint8 INEWS_changeQueue(int qid);
ArticleInfo **INEWS_digest(int n);
Sint8 INEWS_submitArticle(char *title, char *body);
Sint8 INEWS_changeApprovalStatus(Uint32 aid, bool approve);
Sint8 INEWS_changeDeletionStatus(Uint32 aid, bool delete);
void INEWS_disconnect();

Sint8 __read_line(char *str, int max_sz);
Sint8 __write_block(char *str);
char *__chop(char *str);
void *__nop_thread(void *foo);
void __free_queue_info_list_element(/*G*/IList *ptr);
void __print_protocol_fuckery_message();

#endif
