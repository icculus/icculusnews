/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#ifndef __HAVE_ICCULUSNEWS_H
#define __HAVE_ICCULUSNEWS_H

#include <time.h>

#define Uint8 unsigned char
#define Uint16 unsigned short
#define Uint32 unsigned int
#define Uint64 unsigned long long
#define Sint8 signed char
#define Sint16 signed short
#define Sint32 signed int
#define Sint64 signed long long

typedef Uint8 bool;

#define TRUE 1
#define FALSE 0

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
		
// initialize the library
extern int INEWS_init();

// deinitialize the library
extern void INEWS_deinit();

// get the version of the library used at runtime
extern INEWS_Version *INEWS_getVersion();

// get the version of the server daemon
extern char *INEWS_getServerVersion();

// get the current remote host
extern char *INEWS_getHost();

// get the current remote port
extern int INEWS_getPort();

// get the current IcculusNews username
extern const char *INEWS_getUserName();

// get the current IcculusNews user ID
extern int INEWS_getUID();

// get the currently-selected IcculusNews queue ID
extern int INEWS_getQID();

// get detailed information on a chosen queue
extern QueueInfo *INEWS_getQueueInfo(int qid);

// get detailed information on all of the queues
extern QueueInfo **INEWS_getAllQueuesInfo();

// get the error number of the last error to occur
extern int INEWS_getLastError();

// connect to an IcculusNews server
extern int INEWS_connect(const char *hostname, Uint32 port);

// retrieve queue information from the server
extern int INEWS_retrQueueInfo();

// authenticate as a given user to the server
extern int INEWS_auth(const char *username, const char *password);

// change the currently-selected queue
extern int INEWS_changeQueue(int qid);

// retrieve a digest of the currently-selected queue with n articles
extern ArticleInfo **INEWS_digest(int n);

// free the memory dynamically allocated by a call to INEWS_digest
extern void INEWS_freeDigest(ArticleInfo **digest);

// free the memory dynamically allocated by a call to INEWS_getAllQueuesInfo
extern void INEWS_freeQueuesInfo(QueueInfo **qinfo);

// disconnect from the current server
extern void INEWS_disconnect();

#endif
