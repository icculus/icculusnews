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

#ifndef __HAVE_ICCULUSNEWS_H
#define __HAVE_ICCULUSNEWS_H

#define _XOPEN_SOURCE
#include <time.h>

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long long Uint64;
typedef signed char Sint8;
typedef signed short Sint16;
typedef signed int Sint32;
typedef signed long long Sint64;

typedef Uint8 bool;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
    Uint8 major;
    Uint8 minor;
    Uint8 rev;
} INEWS_Version;

typedef struct {
    Uint8 qid;
    char *name;
    char *description;
    char *digest;
    char *singleitem;
    char *home;
    char *rdf;
    struct tm ctime;
    Uint8 owneruid;
    char *ownername;
} QueueInfo;

typedef struct {
    Uint16 aid;
    char *title;
    struct tm ctime;
    Uint8 owneruid;
    char *ownername;
    char *dottedip;
    bool approved;
    bool deleted;
} ArticleInfo;

#define __INEWS_LINKTIME_MAJOR 0
#define __INEWS_LINKTIME_MINOR 0
#define __INEWS_LINKTIME_REV 1

#define INEWS_getLinktimeVersion() \
    { __INEWS_LINKTIME_MAJOR, \
    __INEWS_LINKTIME_MINOR, \
    __INEWS_LINKTIME_REV }

#define ERR_SUCCESS 0 				/* success */
#define ERR_GENERIC -1				/* generic error; blame it on Gen. Protection */
#define ERR_DISCONNECTED -2		/* currently disconnected or connection reset by peer */
#define ERR_NOSUCHQUEUE -3		/* queue does not exist */
#define ERR_NOSUCHUSER -4			/* user does not exist */
#define ERR_UNAUTHORIZED -5		/* I cannot allow you to do that, Dave. */
#define ERR_SERVFAIL -6				/* internal server failure */
#define ERR_STORYTOOLONG -7		/* story was longer than the server wanted it to be; truncated. */
#define ERR_NOSUCHARTICLE -8	/* article does not exist */
		
/* initialize the library */
extern Sint8 INEWS_init();

/* deinitialize the library */
extern void INEWS_deinit();

/* get the version of the library used at runtime */
extern const INEWS_Version *INEWS_getVersion();

/* get the version of the server daemon */
extern inline const char *INEWS_getServerVersion();

/* get the current remote host */
extern inline const char *INEWS_getHost();

/* get the current remote port */
extern inline Uint16 INEWS_getPort();

/* get the current IcculusNews username */
extern inline const char *INEWS_getUserName();

/* get the current IcculusNews user ID */
extern inline Uint16 INEWS_getUID();

/* get the currently-selected IcculusNews queue ID */
extern inline Uint16 INEWS_getQID();

/* get detailed information on a chosen queue */
extern QueueInfo *INEWS_getQueueInfo(int qid);

/* get detailed information on all of the queues */
extern QueueInfo **INEWS_getAllQueuesInfo();

/* get the error number of the last error to occur */
extern inline Sint8 INEWS_getLastError();

/* connect to an IcculusNews server */
extern Sint8 INEWS_connect(const char *hostname, Uint32 port);

/* retrieve queue information from the server */
extern Sint8 INEWS_retrQueueInfo();

/* authenticate as a given user to the server */
extern Sint8 INEWS_auth(const char *username, const char *password);

/* change the currently-selected queue */
extern Sint8 INEWS_changeQueue(int qid);

/* retrieve a digest of the currently-selected queue with n articles */
extern ArticleInfo **INEWS_digest(int offset, int n);

/* submit an article. OMG */
extern Sint8 INEWS_submitArticle(char *title, char *body);

/* submit / edit an article.  supersedes INEWS_submitArticle(). */
extern Sint8 INEWS_submitEditArticle(char *title, char *body, int aid);

/* change the approval status of article aid */
Sint8 INEWS_changeApprovalStatus(Uint32 aid, bool approve);

/* change the deletion status of article aid */
Sint8 INEWS_changeDeletionStatus(Uint32 aid, bool deleteflag);

/* free the memory dynamically allocated by a call to INEWS_digest */
extern void INEWS_freeDigest(ArticleInfo **digest);

/* free the memory dynamically allocated by a call to INEWS_getAllQueuesInfo */
extern void INEWS_freeQueuesInfo(QueueInfo **qinfo);

/* disconnect from the current server */
extern void INEWS_disconnect();

#endif
