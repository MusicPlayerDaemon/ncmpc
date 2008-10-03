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

#ifndef FILELIST_H
#define FILELIST_H

#include <glib.h>

struct mpd_song;

typedef struct filelist_entry {
	guint flags;
	struct mpd_InfoEntity *entity;
} filelist_entry_t;

typedef struct filelist {
	/* path */
	gchar *path;

	/* the list */
	GPtrArray *entries;
} mpdclient_filelist_t;

struct filelist *
filelist_new(const char *path);

void
filelist_free(struct filelist *filelist);

static inline guint
filelist_length(const struct filelist *filelist)
{
	return filelist->entries->len;
}

static inline gboolean
filelist_is_empty(const struct filelist *filelist)
{
	return filelist_length(filelist) == 0;
}

static inline struct filelist_entry *
filelist_get(const struct filelist *filelist, guint i)
{
	return g_ptr_array_index(filelist->entries, i);
}

struct filelist_entry *
filelist_append(struct filelist *filelist, struct mpd_InfoEntity *entity);

struct filelist_entry *
filelist_prepend(struct filelist *filelist, struct mpd_InfoEntity *entity);

void
filelist_move(struct filelist *filelist, struct filelist *from);

void
filelist_sort(struct filelist *filelist, GCompareFunc compare_func);

struct filelist_entry *
filelist_find_song(struct filelist *flist, const struct mpd_song *song);

int
filelist_find_directory(struct filelist *filelist, const char *name);

#endif
