/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#include "internals.h"

Sint8 INEWS_init() {
	pthread_mutex_init(&net_mutex, NULL);
				
	atexit(INEWS_deinit);
	atexit(INEWS_disconnect);
	
	__inews_errno = ERR_SUCCESS;
	return 0;
}

void INEWS_deinit() {
	if (&net_mutex) pthread_mutex_destroy(&net_mutex);
}
