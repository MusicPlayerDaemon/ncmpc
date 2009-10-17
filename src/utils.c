/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "utils.h"
#include "options.h"
#include "charset.h"
#include "i18n.h"
#include "mpdclient.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

GList *
string_list_free(GList *string_list)
{
	GList *list = g_list_first(string_list);

	while (list) {
		g_free(list->data);
		list->data = NULL;
		list = list->next;
	}

	g_list_free(string_list);
	return NULL;
}

GList *
string_list_find(GList *string_list, const gchar *str)
{
	GList *list = g_list_first(string_list);

	while(list) {
		if( strcmp(str, (gchar *) list->data) ==  0 )
			return list;
		list = list->next;
	}
	return NULL;
}

GList *
string_list_remove(GList *string_list, const gchar *str)
{
	GList *list = g_list_first(string_list);

	while(list) {
		if( strcmp(str, (gchar *) list->data) ==  0 ) {
			g_free(list->data);
			list->data = NULL;
			return g_list_delete_link(string_list, list);
		}
		list = list->next;
	}
	return list;
}

/* create a list suitable for GCompletion from path */
GList *
gcmp_list_from_path(struct mpdclient *c, const gchar *path,
		    GList *list, gint types)
{
	struct mpd_connection *connection;
	struct mpd_entity *entity;

	connection = mpdclient_get_connection(c);
	if (connection == NULL)
		return list;

	mpd_send_list_meta(connection, path);

	while ((entity = mpd_recv_entity(connection)) != NULL) {
		char *name;

		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_DIRECTORY &&
		    types & GCMP_TYPE_DIR) {
			const struct mpd_directory *dir =
				mpd_entity_get_directory(entity);
			gchar *tmp = utf8_to_locale(mpd_directory_get_path(dir));
			gsize size = strlen(tmp)+2;

			name = g_malloc(size);
			g_strlcpy(name, tmp, size);
			g_strlcat(name, "/", size);
			g_free(tmp);
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG &&
			   types & GCMP_TYPE_FILE) {
			const struct mpd_song *song =
				mpd_entity_get_song(entity);
			name = utf8_to_locale(mpd_song_get_uri(song));
		} else if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST &&
			   types & GCMP_TYPE_PLAYLIST) {
			const struct mpd_playlist *playlist =
				mpd_entity_get_playlist(entity);
			name = utf8_to_locale(mpd_playlist_get_path(playlist));
		} else {
			mpd_entity_free(entity);
			continue;
		}

		list = g_list_append(list, name);
		mpd_entity_free(entity);
	}

	return list;
}

void
format_duration_short(char *buffer, size_t length, unsigned duration)
{
	if (duration < 3600)
		g_snprintf(buffer, length,
			   "%i:%02i", duration / 60, duration % 60);
	else
		g_snprintf(buffer, length,
			   "%i:%02i:%02i", duration / 3600,
			   (duration % 3600) / 60, duration % 60);
}

void
format_duration_long(char *p, size_t length, unsigned long duration)
{
	const char *year = _("year");
	const char *years = _("years");
	const char *week = _("week");
	const char *weeks = _("weeks");
	const char *day = _("day");
	const char *days = _("days");
	unsigned bytes_written = 0;

	if (duration / 31536000 > 0) {
		if (duration / 31536000 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ", 1, year);
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ", duration / 31536000, years);
		duration %= 31536000;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 604800 > 0) {
		if (duration / 604800 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ", 1, week);
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ", duration / 604800, weeks);
		duration %= 604800;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 86400 > 0) {
		if (duration / 86400 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ", 1, day);
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ", duration / 86400, days);
		duration %= 86400;
		length -= bytes_written;
		p += bytes_written;
	}

	g_snprintf(p, length, "%02lu:%02lu:%02lu", duration / 3600,
		   duration % 3600 / 60, duration % 3600 % 60);
}
