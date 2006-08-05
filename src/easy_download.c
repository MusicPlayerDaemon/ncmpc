/*	by Qball
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "ncmpc.h"

#ifdef ENABLE_LYRICS_SCREEN

#include <curl/curl.h>
#include "easy_download.h"

static size_t write_data(void *buffer, size_t size,
	size_t nmemb, easy_download_struct *dld)
{
	if(dld->data == NULL)
	{
		dld->size = 0;
	}
	dld->data = g_realloc(dld->data,(gulong)(size*nmemb+dld->size));

	memset(&(dld->data)[dld->size], '\0', (size*nmemb));
	memcpy(&(dld->data)[dld->size], buffer, size*nmemb);

	dld->size += size*nmemb;
	if(dld->size >= dld->max_size && dld->max_size > 0)
	{
		return 0;
	}
	return size*nmemb;
}

int easy_download(char *url,easy_download_struct *dld,
		curl_progress_callback cb)
{
	CURL *curl = NULL;
	int res;
	if(!dld) return 0;
	easy_download_clean(dld);
	/* initialize curl */
	curl = curl_easy_init();
	if(!curl) return 0;
	/* set uri */
	curl_easy_setopt(curl, CURLOPT_URL, url);
	/* set callback data */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, dld);
	/* set callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, cb);
	/* run */
	res = curl_easy_perform(curl);
	/* cleanup */
	curl_easy_cleanup(curl);
	if(!res)
	{
		return 1;
	}
	if(dld->data) g_free(dld->data);
	return 0;
}

void easy_download_clean(easy_download_struct *dld)
{
	if(dld->data)g_free(dld->data);
	dld->data = NULL;
	dld->size = 0;
	dld->max_size = -1;
}

#endif
