/* LibINews -- the only IcculusNews backend with the power of nougat
 * copyright (c) 2002 Colin "vogon" Bayer
 *
 * [ -- Insert GPL boilerplate here -- ]
 *
 */

#include "internals.h"

int INEWS_init() {
	g_thread_init(NULL);

	if (!g_thread_supported()) return ERR_GENERIC;

	g_static_mutex_init(&net_mutex);
	
	atexit(INEWS_deinit);
	atexit(INEWS_disconnect);
	
	return ERR_SUCCESS;
}

void INEWS_deinit() {
	if (&net_mutex) g_static_mutex_free(&net_mutex);
}
