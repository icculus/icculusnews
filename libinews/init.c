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
