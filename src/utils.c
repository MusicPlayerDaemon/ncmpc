/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "utils.h"
#include "i18n.h"

#include <stdlib.h>
#include <string.h>

GList *
string_list_free(GList *string_list)
{
	g_list_free_full(string_list, g_free);
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
	unsigned bytes_written = 0;

	if (duration / 31536000 > 0) {
		if (duration / 31536000 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ", 1, _("year"));
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ", duration / 31536000, _("years"));
		duration %= 31536000;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 604800 > 0) {
		if (duration / 604800 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ",
						   1, _("week"));
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ",
						   duration / 604800, _("weeks"));
		duration %= 604800;
		length -= bytes_written;
		p += bytes_written;
	}
	if (duration / 86400 > 0) {
		if (duration / 86400 == 1)
			bytes_written = g_snprintf(p, length, "%d %s, ",
						   1, _("day"));
		else
			bytes_written = g_snprintf(p, length, "%lu %s, ",
						   duration / 86400, _("days"));
		duration %= 86400;
		length -= bytes_written;
		p += bytes_written;
	}

	g_snprintf(p, length, "%02lu:%02lu:%02lu", duration / 3600,
		   duration % 3600 / 60, duration % 3600 % 60);
}
