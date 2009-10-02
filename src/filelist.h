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

#ifndef FILELIST_H
#define FILELIST_H

#include <glib.h>

struct mpd_connection;
struct mpd_song;

struct filelist_entry {
	guint flags;
	struct mpd_entity *entity;
};

struct filelist {
	/* the list */
	GPtrArray *entries;
};

struct filelist *
filelist_new(void);

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
filelist_append(struct filelist *filelist, struct mpd_entity *entity);

struct filelist_entry *
filelist_prepend(struct filelist *filelist, struct mpd_entity *entity);

void
filelist_move(struct filelist *filelist, struct filelist *from);

/* Sorts the whole filelist, at the moment used by filelist_search */
void
filelist_sort_all(struct filelist *filelist, GCompareFunc compare_func);

/* Only sorts the directories and playlist files.
 * The songs stay in the order it came from mpd. */
void
filelist_sort_dir_play(struct filelist *filelist, GCompareFunc compare_func);

int
filelist_find_song(struct filelist *flist, const struct mpd_song *song);

int
filelist_find_directory(struct filelist *filelist, const char *name);

/**
 * Receives entities from the connection, and appends them to the
 * specified filelist.  This does not finish the response, and does
 * not check for errors.
 */
void
filelist_recv(struct filelist *filelist, struct mpd_connection *connection);

/**
 * Creates a new filelist and receives entities from the connection.
 * This does not finish the response, and does not check for errors.
 */
struct filelist *
filelist_new_recv(struct mpd_connection *connection);

#endif
