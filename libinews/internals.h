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
#include <pth.h>

/* squelch all the implicit function decl warnings */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>

/* semaphore, other IPC crap */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

/* we use glib, because I'm too lazy to write 
 * string-mangling functions myself */
#include <glib-2.0/glib.h>

/* semaphore union */
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* our work is done for us */
#else
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo *__buf;
};
#endif

#define INEWS_MAJOR __INEWS_LINKTIME_MAJOR
#define INEWS_MINOR __INEWS_LINKTIME_MINOR
#define INEWS_REV __INEWS_LINKTIME_REV

INEWS_Version __inews_version;

typedef struct {
	gboolean connected;
	char *hostname;
	Uint16 port;
	char *username;
	char *password;
	Uint8 uid;
	Uint8 qid;
	char *verstring;
} ServerState;

ServerState serverstate;
GList *qinfoptr;
int fd;
struct sockaddr_in sa;
size_t sa_len;
int keep_nopping;
int __inews_errno;

/* GStaticMutex net_mutex; */
int net_sem;

/* GThread *nop_thread_ptr; */
pth_t nop_thread;

Sint8 INEWS_init();
void INEWS_deinit();

INEWS_Version *INEWS_getVersion();
char *INEWS_getServerVersion();
char *INEWS_getHost();
Uint16 INEWS_getPort();
const char *INEWS_getUserName();
Uint16 INEWS_getUID();
Uint16 INEWS_getQID();
QueueInfo *INEWS_getQueueInfo(int qid);
QueueInfo **INEWS_getAllQueuesInfo();
Sint8 INEWS_getLastError();

void INEWS_freeDigest(ArticleInfo **digest);
void INEWS_freeQueuesInfo(QueueInfo **qinfo);

Sint8 INEWS_connect(const char *hostname, Uint32 port);
Sint8 INEWS_auth(const char *username, const char *password);
Sint8 INEWS_retrQueueInfo();
Sint8 INEWS_changeQueue(int qid);
ArticleInfo **INEWS_digest(int n);
Sint8 INEWS_submitArticle(char *title, char *body);
void INEWS_disconnect();

Sint8 __read_line(char *str, int max_sz);
Sint8 __write_block(char *str);
char *__chop(char *str);
void *__nop_thread(void *foo);
void __free_queue_info_list_element(GList *ptr);
void __print_protocol_fuckery_message();

/* semaphore functions */
Sint8 __sem_lock(int sem);
Sint8 __sem_unlock(int sem);

#endif
