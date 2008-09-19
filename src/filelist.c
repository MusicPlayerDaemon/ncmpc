/*
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
 * (c) 2008 Max Kellermann <max@duempel.org>
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

#include "filelist.h"
#include "libmpdclient.h"

#include <string.h>
#include <assert.h>

void
filelist_free(struct filelist *filelist)
{
	GList *list = g_list_first(filelist->list);

	if (list == NULL)
		return;

	while (list != NULL) {
		filelist_entry_t *entry = list->data;

		if (entry->entity)
			mpd_freeInfoEntity(entry->entity);

		g_free(entry);
		list = list->next;
	}

	g_list_free(filelist->list);
	g_free(filelist->path);
	g_free(filelist);
}

struct filelist_entry *
filelist_find_song(struct filelist *fl, const struct mpd_song *song)
{
	GList *list = g_list_first(fl->list);

	assert(song != NULL);

	while (list != NULL) {
		filelist_entry_t *entry = list->data;
		mpd_InfoEntity *entity  = entry->entity;

		if (entity && entity->type == MPD_INFO_ENTITY_TYPE_SONG) {
			struct mpd_song *song2 = entity->info.song;

			if (strcmp(song->file, song2->file) == 0)
				return entry;
		}

		list = list->next;
	}

	return NULL;
}
