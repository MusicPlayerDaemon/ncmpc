/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#ifndef UTILS_H
#define UTILS_H

#include <glib.h>

struct mpdclient;

/* functions for lists containing strings */
GList *string_list_free(GList *string_list);
GList *string_list_find(GList *string_list, const gchar *str);
GList *string_list_remove(GList *string_list, const gchar *str);

/* create a string list from path - used for completion */
#define GCMP_TYPE_DIR       (0x01 << 0)
#define GCMP_TYPE_FILE      (0x01 << 1)
#define GCMP_TYPE_PLAYLIST  (0x01 << 2)
#define GCMP_TYPE_RFILE     (GCMP_TYPE_DIR | GCMP_TYPE_FILE)
#define GCMP_TYPE_RPLAYLIST (GCMP_TYPE_DIR | GCMP_TYPE_PLAYLIST)

GList *
gcmp_list_from_path(struct mpdclient *c, const gchar *path,
		    GList *list, gint types);

void
format_duration_short(char *buffer, size_t length, unsigned duration);

void
format_duration_long(char *buffer, size_t length, unsigned long duration);

#endif
