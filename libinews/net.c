/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#include "internals.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#define _XOPEN_SOURCE
#include <time.h>

int INEWS_connect(const char *hostname, Uint32 port) {
	struct hostent *hostents;
	Uint32 err;
	Sint32 read_ret;
	
	g_static_mutex_lock(&net_mutex);
	
	serverstate.hostname = g_strdup(hostname);
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
		return ERR_GENERIC;
	}

	sa_len = sizeof(sa);
	memset(&sa, 0, sa_len);
	
	memcpy(&sa.sin_addr, hostents->h_addr_list[0], hostents->h_length);
	sa.sin_port = htons(port);
	sa.sin_family = AF_INET;
	
	if (!(fd = socket(PF_INET, SOCK_STREAM, 0))) {
		printf("Error returned from socket(): %s\n", strerror(errno));
		return ERR_GENERIC;
	}

	if (connect(fd, (struct sockaddr *)&sa, sa_len) != 0) {
		printf("Error returned from connect(): %s\n", strerror(errno));
		return ERR_GENERIC;
	}

	fcntl(fd, F_SETFL, (fcntl(fd, F_GETFL) | O_NONBLOCK));

	{
		char welcome_msg[80];

		memset(welcome_msg, 0, 80);
	
		if ((read_ret = __read_line(welcome_msg, 80)) == 0) {
			char *offset;
			
			if ((offset = strstr((char *)welcome_msg, "daemon")) != NULL)
				serverstate.verstring = g_strdup(offset + 7);
		} else {
			g_static_mutex_unlock(&net_mutex);
			return read_ret;
		}
	}
	
	g_static_mutex_unlock(&net_mutex);

	serverstate.connected = TRUE;

	return ERR_SUCCESS;
}

int INEWS_auth(const char *username, const char *password) {
	char authstring[255];
	char respstring[81];

	if (!serverstate.connected) return ERR_DISCONNECTED;
	
	memset(authstring, 0, 255);
	memset(respstring, 0, 81);
	
	g_static_mutex_lock(&net_mutex);

	if (username) {
		sprintf(authstring, "AUTH \"%s\" \"%s\"\n", username, password);
	} else {
		strcat(authstring, "AUTH -\n");
	}

	__write_block((char *)authstring);

	if (__read_line((char *)respstring, 80) == -2) {
		g_static_mutex_unlock(&net_mutex);
		return ERR_DISCONNECTED;
	}

	g_static_mutex_unlock(&net_mutex);	
	
	switch(respstring[0]) {
		case '+':
			printf("Authorization successful.\n");
			serverstate.username = username ? g_strdup(username) : g_strdup("anonymous");
			serverstate.password = password ? g_strdup(password) : NULL;
			serverstate.uid = atoi(respstring + 2);
			serverstate.qid = atoi(strchr(respstring, ',') + 1);
			keep_nopping = 1;
			nop_thread_ptr = g_thread_create(__nop_thread, NULL, TRUE, NULL);
			return ERR_SUCCESS;
			break;
		default:
			switch(respstring[2]) {
				case 'c': /* "can't execute query" */
					INEWS_disconnect();
					return ERR_SERVFAIL;
					break;
				case 'A': /* "Authorization for <foo> failed." */
					return ERR_NOSUCHUSER;
					break;
				case 'T': /* "This account has been disabled." */
					return ERR_UNAUTHORIZED;
					break;
				case 'F': /* "Failed to set default queue" */
					return ERR_NOSUCHQUEUE;
					break;
				default:
					printf("Someone fucked with the protocol in INEWS_auth().\n");
					printf("Flame <vogon@icculus.org> about it.\n");
					return ERR_GENERIC;
					break;
			}
	}
}

int INEWS_retrQueueInfo() {
	QueueInfo *temp_ptr;
	char temp_data[256];
	GList *temp_iter;
	Uint8 record_pos = 0;
	Sint32 read_ret;

	if (!serverstate.connected) return ERR_DISCONNECTED;
	
	g_static_mutex_lock(&net_mutex);

	__write_block("ENUM queues\n");
	if ((read_ret = __read_line(temp_data, 255)) < 0) { /* to get rid of "+ Here comes..." */
		g_static_mutex_unlock(&net_mutex);
		return read_ret;
	}
					
	while (1) {
		memset(temp_data, 0, 256);
		if ((read_ret = __read_line(temp_data, 255)) < 0) {
			g_static_mutex_unlock(&net_mutex);
			return read_ret;
		}

		if (!strcmp(temp_data, ".")) break;
					
		if (!record_pos) {
			temp_ptr = (QueueInfo *)malloc(sizeof(QueueInfo));
			temp_ptr->qid = atoi(temp_data);
			record_pos = 1;
		} else {
			temp_ptr->name = g_strdup(temp_data);
			qinfoptr = g_list_append(qinfoptr, temp_ptr);
			record_pos = 0;
		}
	}

	temp_iter = qinfoptr;
	
	while (temp_iter) {
		QueueInfo *temp_qinfo = (QueueInfo *)temp_iter->data;
		memset(temp_data, 0, 256);
		
		sprintf(temp_data, "QUEUEINFO %hhi\n", temp_qinfo->qid);

		__write_block(temp_data);
		
		if ((read_ret = __read_line(temp_data, 255)) < 0) {
			g_static_mutex_unlock(&net_mutex);
			return read_ret;
		}

		record_pos = 0;
		
		while (record_pos < 9) {
			memset(temp_data, 0, 256);
			
			if ((read_ret = __read_line(temp_data, 255)) < 0) {
				g_static_mutex_unlock(&net_mutex);
				return read_ret;
			}
			
			switch (++record_pos) {
				case 2: temp_qinfo->description = g_strdup(temp_data);
								break;
				case 3: temp_qinfo->digest = g_strdup(temp_data); 
								break;
				case 4: temp_qinfo->singleitem = g_strdup(temp_data); 
								break;
				case 5: temp_qinfo->home = g_strdup(temp_data); break;
				case 6: temp_qinfo->rdf = g_strdup(temp_data); break;
				case 7: strptime(temp_data, "%Y-%m-%d %T", &(temp_qinfo->ctime));
								break;
				case 8: temp_qinfo->owneruid = atoi(temp_data); break;
				case 9: temp_qinfo->ownername = g_strdup(temp_data); break;
			}
		}

		temp_iter = g_list_next(temp_iter);
	}

	g_static_mutex_unlock(&net_mutex);
}

int INEWS_changeQueue(int qid) {
	char tempstring[255];
	Sint32 read_ret;

	if (!serverstate.connected) return ERR_DISCONNECTED;
	if (qid == serverstate.qid) return ERR_SUCCESS; /* no-op */
	
	memset(tempstring, 0, 255);
				
	g_static_mutex_lock(&net_mutex);
	
	sprintf(tempstring, "QUEUE %i\n", qid);

	__write_block(tempstring);

	memset(tempstring, 0, 255);

	if ((read_ret = __read_line(tempstring, 254)) < 0) {
		g_static_mutex_unlock(&net_mutex);
		return read_ret;
	}

	g_static_mutex_unlock(&net_mutex);
	
	switch(tempstring[0]) {
		case '+':
			serverstate.qid = qid;
			return ERR_SUCCESS;
			break;
		case '-':
			switch (tempstring[2]) {
				case 'c': /* "can't execute query" */
					INEWS_disconnect();
					return ERR_SERVFAIL;
					break;
				case 'C': /* "Can't select that queue" */
					return ERR_NOSUCHQUEUE;
					break;
				default:
					printf("Someone fucked with the protocol in INEWS_changeQueue().\n");
					printf("Flame <vogon@icculus.org> about it.\n");
					return ERR_GENERIC;
					break;
			}
	}
}

ArticleInfo **INEWS_digest(int n) {
	char tempstring[256];
	ArticleInfo **retval;
	ArticleInfo *tempinfo;
	int read_ret, record_pos, count = 0;
	bool eor = FALSE;
	
	if (!serverstate.connected) {
		__inews_errno = ERR_DISCONNECTED;
		return NULL;
	}
	
	retval = (ArticleInfo **)malloc(n * sizeof(ArticleInfo *));
	
	g_static_mutex_lock(&net_mutex);

	sprintf(tempstring, "DIGEST %i\n", n);

	__write_block(tempstring);
	
	if ((read_ret = __read_line(tempstring, 255)) < 0) { /* to get rid of "+ Here comes..." */
	  g_static_mutex_unlock(&net_mutex);
	  __inews_errno = read_ret;
		return NULL;
	}
	
	while (count < n) {
		tempinfo = (ArticleInfo *)malloc(sizeof(ArticleInfo));
					
		record_pos = 0;
										
		while (record_pos < 8) {
			memset(tempstring, 0, 256);
						
			if ((read_ret = __read_line(tempstring, 255)) < 0) {
				g_static_mutex_unlock(&net_mutex);
				__inews_errno = read_ret;
				return NULL;
			}

			if (!strcmp(tempstring, ".")) {
				eor = TRUE;
				break;
			}

			switch (++record_pos) {
				case 1: tempinfo->aid = atoi(tempstring); break;
				case 2: tempinfo->title = g_strdup(tempstring); break;
				case 3: strptime(tempstring, "%Y-%m-%d %T", &(tempinfo->ctime));
								break;
				case 4: tempinfo->owneruid = atoi(tempstring); break;
				case 5: tempinfo->ownername = g_strdup(tempstring); break;
				case 6: tempinfo->dottedip = g_strdup(tempstring); break;
				case 7: tempinfo->approved = atoi(tempstring); break;
				case 8: tempinfo->deleted = atoi(tempstring); break;
			}
		}

		if (eor) break;
		
		retval[count++] = tempinfo;
	}
	
	g_static_mutex_unlock(&net_mutex);

	/* if we hit a premature end-of-record, then we'll need to shrink down our 
	 * return to prevent problems when we try to free it */
	retval = realloc(retval, (count * sizeof(ArticleInfo *)));
	
	return retval;
}

void INEWS_disconnect() {
	GList *qinfo_iter = qinfoptr;

	if (!serverstate.connected) return;
	
	keep_nopping = 0;
	if (nop_thread_ptr && nop_thread_ptr->joinable) 
		g_thread_join(nop_thread_ptr);
	close(fd);
	serverstate.connected = FALSE;

	// clean up dynamically-allocated crap.
	free(serverstate.hostname);
	free(serverstate.username);
	free(serverstate.password);
	free(serverstate.verstring);

	if (!qinfo_iter) return;
	
	while (1) {
		__free_queue_info_list_element(qinfo_iter);
		qinfo_iter = g_list_remove_link(qinfo_iter, qinfo_iter);
		if (!qinfo_iter) break;
	}
}

/* internal: returns one line of text from the server,
 * /sans/ the newline. */

int __read_line(char *str, int max_sz) {
	ssize_t count = 0, len = 0;

	while (len < max_sz) {
		count = read(fd, (str + len), 1);
		if (count < 0 && (errno != EAGAIN && errno != EINTR)) {
			printf("Error from read(): %s\n", strerror(errno));
			return ERR_GENERIC;
		} else if (count == 0) {
			printf("Connection reset by peer\n");
			INEWS_disconnect();
			return ERR_DISCONNECTED;
		} else if (count > 0) {
			len += count;
			if (*(str + len - 1) == '\n')	break;
		}
	}

	__chop(str);
	
	return ERR_SUCCESS;
}

int __write_block(char *str) {
	ssize_t count = 0, len = 0;
	
	while (len < strlen(str)) {
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

gpointer __nop_thread(gpointer foo) {
	char throwaway[81];
	Sint32 read_ret;
	
	while (keep_nopping) {
		sleep(2);
		g_static_mutex_lock(&net_mutex);

		__write_block("NOOP\n");
		if ((read_ret = __read_line(throwaway, 80)) < 0) {
			g_static_mutex_unlock(&net_mutex);
			return NULL;
		}
		
		g_static_mutex_unlock(&net_mutex);
	}

	return NULL;
}
