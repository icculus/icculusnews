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

/* squelch all the implicit function decl warnings */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <execinfo.h>

/* we use glib, because I'm too lazy to write 
 * string-mangling functions myself */
#include <glib-2.0/glib.h>

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

GStaticMutex net_mutex;

GThread *nop_thread_ptr;

int INEWS_init();
void INEWS_deinit();

INEWS_Version *INEWS_getVersion();
char *INEWS_getServerVersion();
char *INEWS_getHost();
int INEWS_getPort();
const char *INEWS_getUserName();
int INEWS_getUID();
int INEWS_getQID();
QueueInfo *INEWS_getQueueInfo(int qid);
QueueInfo **INEWS_getAllQueuesInfo();
int INEWS_getLastError();

void INEWS_freeDigest(ArticleInfo **digest);
void INEWS_freeQueuesInfo(QueueInfo **qinfo);

int INEWS_connect(const char *hostname, Uint32 port);
int INEWS_auth(const char *username, const char *password);
int INEWS_retrQueueInfo();
int INEWS_changeQueue(int qid);
ArticleInfo **INEWS_digest(int n);
int INEWS_submitArticle(char *title, char *body);
void INEWS_disconnect();

int __read_line(char *str, int max_sz);
int __write_block(char *str);
char *__chop(char *str);
gpointer __nop_thread(gpointer foo);
void __free_queue_info_list_element(GList *ptr);
void __print_protocol_fuckery_message();

#endif
