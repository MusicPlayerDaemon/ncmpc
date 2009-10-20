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

#ifndef NCMPC_SONG_PAINT_H
#define NCMPC_SONG_PAINT_H

#include <stdbool.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

struct mpd_song;
struct hscroll;

/**
 * Paints a song into a list window row.  The cursor must be set to
 * the first character in the row prior to calling this function.
 *
 * @param w the ncurses window
 * @param y the row number in the window
 * @param width the width of the row
 * @param selected true if the row is selected
 * @param highlight true if the row is highlighted
 * @param song the song object
 * @param hscroll an optional hscroll object
 */
void
paint_song_row(WINDOW *w, unsigned y, unsigned width,
	       bool selected, bool highlight, const struct mpd_song *song,
	       struct hscroll *hscroll);

#endif
