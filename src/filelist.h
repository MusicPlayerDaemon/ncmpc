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

	/* list length */
	guint length;

	/* true if the list is updated */
	gboolean updated;

	/* the list */
	GList *list;
} mpdclient_filelist_t;

void
mpdclient_filelist_free(struct filelist *filelist);

struct filelist_entry *
mpdclient_filelist_find_song(struct filelist *flist,
			     const struct mpd_song *song);

#endif
