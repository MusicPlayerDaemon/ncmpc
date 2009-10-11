/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "song_paint.h"
#include "paint.h"
#include "strfsong.h"
#include "utils.h"
#include "config.h"

#include <mpd/client.h>

#include <glib.h>

#include <string.h>

void
paint_song_row(WINDOW *w, G_GNUC_UNUSED unsigned y, unsigned width,
	       bool selected, bool highlight, const struct mpd_song *song)
{
	char buffer[width * 4];

	strfsong(buffer, sizeof(buffer), options.list_format, song);
	row_paint_text(w, width, highlight ? COLOR_LIST_BOLD : COLOR_LIST,
		       selected, buffer);

#ifndef NCMPC_MINI
	if (mpd_song_get_duration(song) > 0) {
		char duration[32];
		format_duration_short(duration, sizeof(duration),
				      mpd_song_get_duration(song));
		wmove(w, y, width - strlen(duration) - 1);
		waddch(w, ' ');
		waddstr(w, duration);
	}
#endif
}
