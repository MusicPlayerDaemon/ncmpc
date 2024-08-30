// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_SONG_ROW_PAINT_HXX
#define NCMPC_SONG_ROW_PAINT_HXX

#include <curses.h>

struct mpd_song;
class hscroll;

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
 * @param format the song format
 */
void
paint_song_row(WINDOW *w, int y, unsigned width,
	       bool selected, bool highlight, const struct mpd_song *song,
	       class hscroll *hscroll, const char *format);

#endif
