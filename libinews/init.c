/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#include "internals.h"

Sint8 INEWS_init() {
	/* g_thread_init(NULL); */
	if (!pth_init()) {
		__inews_errno = ERR_GENERIC;
		return -1;
	}

	/*if (!g_thread_supported()) {
		__inews_errno = ERR_GENERIC;
		return -1;
	}*/

	if (((net_sem = semget((key_t)31415, 1, 0666 | IPC_CREAT | IPC_EXCL)) < 0) ||
			(semctl(net_sem, 0, SETVAL, 1) < 0)) {
		printf("Guru meditation error: couldn't initialize semaphore 0x%x (%s)\n",
					 net_sem, strerror(errno));
		__inews_errno = ERR_GENERIC;
		return -1;
	}
	
	/* g_static_mutex_init(&net_mutex); */
	
	atexit(INEWS_deinit);
	atexit(INEWS_disconnect);
	
	__inews_errno = ERR_SUCCESS;
	return 0;
}

void INEWS_deinit() {
	/*if (&net_mutex) g_static_mutex_free(&net_mutex);*/
	if (net_sem) semctl(net_sem, 0, IPC_RMID);
	pth_kill();
}
