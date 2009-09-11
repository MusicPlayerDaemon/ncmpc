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

#ifndef SCREEN_H
#define SCREEN_H

#include "config.h"
#include "mpdclient.h"
#include "command.h"

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#define IS_PLAYING(s) (s==MPD_STATUS_STATE_PLAY)
#define IS_PAUSED(s) (s==MPD_STATUS_STATE_PAUSE)
#define IS_STOPPED(s) (!(IS_PLAYING(s) | IS_PAUSED(s)))

#define MAX_SONGNAME_LENGTH   512

struct window {
	WINDOW *w;
	unsigned rows, cols;
};

struct screen {
	struct window top_window;
	struct window main_window;
	struct window progress_window;
	struct window status_window;

	/* GTime is equivalent to time_t */
	GTime start_timestamp;
	GTime status_timestamp;

	command_t last_cmd;

	unsigned cols, rows;

	char *buf;
	size_t buf_size;

	char *findbuf;
	GList *find_history;
};

extern struct screen screen;

extern const struct screen_functions screen_playlist;
extern const struct screen_functions screen_browse;
#ifdef ENABLE_ARTIST_SCREEN
extern const struct screen_functions screen_artist;
#endif
extern const struct screen_functions screen_help;
#ifdef ENABLE_SEARCH_SCREEN
extern const struct screen_functions screen_search;
#endif
#ifdef ENABLE_SONG_SCREEN
extern const struct screen_functions screen_song;
#endif
#ifdef ENABLE_KEYDEF_SCREEN
extern const struct screen_functions screen_keydef;
#endif
#ifdef ENABLE_LYRICS_SCREEN
extern const struct screen_functions screen_lyrics;
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
extern const struct screen_functions screen_outputs;
#endif


typedef struct screen_functions {
	void (*init)(WINDOW *w, int cols, int rows);
	void (*exit)(void);
	void (*open)(mpdclient_t *c);
	void (*close)(void);
	void (*resize)(int cols, int rows);
	void (*paint)(void);
	void (*update)(mpdclient_t *c);
	bool (*cmd)(mpdclient_t *c, command_t cmd);
	const char *(*get_title)(char *s, size_t size);
} screen_functions_t;

void screen_init(mpdclient_t *c);
void screen_exit(void);
void screen_resize(struct mpdclient *c);
void screen_status_message(const char *msg);
void screen_status_printf(const char *format, ...);
char *screen_error(void);
void screen_paint(mpdclient_t *c);
void screen_update(mpdclient_t *c);
void screen_idle(mpdclient_t *c);
void screen_cmd(mpdclient_t *c, command_t cmd);
gint screen_get_id(const char *name);

void
screen_switch(const struct screen_functions *sf, struct mpdclient *c);
void 
screen_swap(struct mpdclient *c, const struct mpd_song *song);

gboolean
screen_is_visible(const struct screen_functions *sf);

int screen_get_mouse_event(mpdclient_t *c, unsigned long *bstate, int *row);

bool
screen_file_goto_song(struct mpdclient *c, const struct mpd_song *song);

#ifdef ENABLE_SONG_SCREEN
void
screen_song_switch(struct mpdclient *c, const struct mpd_song *song);
#endif

#ifdef ENABLE_LYRICS_SCREEN
void
screen_lyrics_switch(struct mpdclient *c, const struct mpd_song *song, bool follow);
#endif

#endif
