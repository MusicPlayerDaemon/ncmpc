/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "utils.h"
#include "options.h"
#include "support.h"

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

/* create a list suiteble for GCompletion from path */
GList *
gcmp_list_from_path(mpdclient_t *c, const gchar *path, GList *list, gint types)
{
	guint i;
	mpdclient_filelist_t *filelist;

	if ((filelist = mpdclient_filelist_get(c, path)) == NULL)
		return list;

	for (i = 0; i < filelist_length(filelist); ++i) {
		struct filelist_entry *entry = filelist_get(filelist, i);
		mpd_InfoEntity *entity = entry ? entry->entity : NULL;
		char *name = NULL;

		if (entity && entity->type==MPD_INFO_ENTITY_TYPE_DIRECTORY &&
		    types & GCMP_TYPE_DIR) {
			mpd_Directory *dir = entity->info.directory;
			gchar *tmp = utf8_to_locale(dir->path);
			gsize size = strlen(tmp)+2;

			name = g_malloc(size);
			g_strlcpy(name, tmp, size);
			g_strlcat(name, "/", size);
			g_free(tmp);
		} else if (entity &&
			   entity->type == MPD_INFO_ENTITY_TYPE_SONG &&
			   types & GCMP_TYPE_FILE) {
			mpd_Song *song = entity->info.song;
			name = utf8_to_locale(song->file);
		} else if (entity &&
			   entity->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE &&
			   types & GCMP_TYPE_PLAYLIST) {
			mpd_PlaylistFile *plf = entity->info.playlistFile;
			name = utf8_to_locale(plf->path);
		}

		if (name)
			list = g_list_append(list, name);
	}

	filelist_free(filelist);
	return list;
}
