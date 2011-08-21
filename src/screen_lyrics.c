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

#include "screen_lyrics.h"
#include "screen_interface.h"
#include "screen_message.h"
#include "screen_file.h"
#include "screen_song.h"
#include "i18n.h"
#include "options.h"
#include "mpdclient.h"
#include "screen.h"
#include "lyrics.h"
#include "screen_text.h"

#include <assert.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>

static struct screen_text text;

static struct mpd_song *next_song;
static bool follow = false;

static struct {
	struct mpd_song *song;

	char *artist, *title, *plugin_name;

	struct plugin_cycle *loader;

	guint loader_timeout;
} current;

static void
screen_lyrics_abort(void)
{
	if (current.loader != NULL) {
		plugin_stop(current.loader);
		current.loader = NULL;
	}

	if (current.loader_timeout != 0) {
		g_source_remove(current.loader_timeout);
		current.loader_timeout = 0;
	}

	if (current.plugin_name != NULL) {
		g_free(current.plugin_name);
		current.plugin_name = NULL;
	}

	if (current.artist != NULL) {
		g_free(current.artist);
		current.artist = NULL;
	}

	if (current.title != NULL) {
		g_free(current.title);
		current.title = NULL;
	}

	if (current.song != NULL) {
		mpd_song_free(current.song);
		current.song = NULL;
	}
}

/**
 * Repaint and update the screen, if it is currently active.
 */
static void
lyrics_repaint_if_active(void)
{
	if (screen_is_visible(&screen_lyrics)) {
		screen_text_repaint(&text);

		/* XXX repaint the screen title */
	}
}

static void
path_lyr_file(char *path, size_t size,
		const char *artist, const char *title)
{
	snprintf(path, size, "%s/.lyrics/%s - %s.txt",
			getenv("HOME"), artist, title);
}

static bool
exists_lyr_file(const char *artist, const char *title)
{
	char path[1024];
	struct stat result;

	path_lyr_file(path, 1024, artist, title);

	return (stat(path, &result) == 0);
}

static FILE *
create_lyr_file(const char *artist, const char *title)
{
	char path[1024];

	snprintf(path, 1024, "%s/.lyrics",
		 getenv("HOME"));
	mkdir(path, S_IRWXU);

	path_lyr_file(path, 1024, artist, title);

	return fopen(path, "w");
}

static int
store_lyr_hd(void)
{
	FILE *lyr_file;
	unsigned i;

	lyr_file = create_lyr_file(current.artist, current.title);
	if (lyr_file == NULL)
		return -1;

	for (i = 0; i < text.lines->len; ++i)
		fprintf(lyr_file, "%s\n",
			(const char*)g_ptr_array_index(text.lines, i));

	fclose(lyr_file);
	return 0;
}

static int
delete_lyr_hd(void)
{
	char path[1024];

	if (!exists_lyr_file(current.artist, current.title))
		return -1;

	path_lyr_file(path, 1024, current.artist, current.title);
	if (unlink(path) != 0)
		return -2;

	return 0;
}

static void
screen_lyrics_set(const GString *str)
{
	screen_text_set(&text, str);

	/* paint new data */

	lyrics_repaint_if_active();
}

static void
screen_lyrics_callback(const GString *result, const bool success,
		       const char *plugin_name, G_GNUC_UNUSED void *data)
{
	assert(current.loader != NULL);

	current.plugin_name = g_strdup(plugin_name);

	/* Display result, which may be lyrics or error messages */
	if (result != NULL)
		screen_lyrics_set(result);

	if (success == true) {
		if (options.lyrics_autosave &&
		    !exists_lyr_file(current.artist, current.title))
			store_lyr_hd();
	} else {
		/* translators: no lyrics were found for the song */
		screen_status_message (_("No lyrics"));
	}

	if (current.loader_timeout != 0) {
		g_source_remove(current.loader_timeout);
		current.loader_timeout = 0;
	}

	plugin_stop(current.loader);
	current.loader = NULL;
}

static gboolean
screen_lyrics_timeout_callback(gpointer G_GNUC_UNUSED data)
{
	plugin_stop(current.loader);
	current.loader = NULL;

	screen_status_printf(_("Lyrics timeout occurred after %d seconds"),
			     options.lyrics_timeout);

	current.loader_timeout = 0;
	return FALSE;
}

static void
screen_lyrics_load(const struct mpd_song *song)
{
	const char *artist, *title;

	assert(song != NULL);

	screen_lyrics_abort();
	screen_text_clear(&text);

	artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

	current.song = mpd_song_dup(song);
	current.artist = g_strdup(artist);
	current.title = g_strdup(title);

	current.loader = lyrics_load(current.artist, current.title,
				     screen_lyrics_callback, NULL);

	if (options.lyrics_timeout != 0) {
		current.loader_timeout =
			g_timeout_add_seconds(options.lyrics_timeout,
					      screen_lyrics_timeout_callback,
					      NULL);
	}
}

static void
lyrics_screen_init(WINDOW *w, int cols, int rows)
{
	screen_text_init(&text, w, cols, rows);
}

static void
lyrics_resize(int cols, int rows)
{
	screen_text_resize(&text, cols, rows);
}

static void
lyrics_exit(void)
{
	screen_lyrics_abort();

	screen_text_deinit(&text);
}

static void
lyrics_open(struct mpdclient *c)
{
	const struct mpd_song *next_song_c =
		next_song != NULL ? next_song : c->song;

	if (next_song_c != NULL &&
	    (current.song == NULL ||
	     strcmp(mpd_song_get_uri(next_song_c),
		    mpd_song_get_uri(current.song)) != 0))
		screen_lyrics_load(next_song_c);

	if (next_song != NULL) {
		mpd_song_free(next_song);
		next_song = NULL;
	}
}

static void
lyrics_update(struct mpdclient *c)
{
	if (!follow)
		return;

	if (c->song != NULL &&
	    (current.song == NULL ||
	     strcmp(mpd_song_get_uri(c->song),
		    mpd_song_get_uri(current.song)) != 0))
		screen_lyrics_load(c->song);
}

static const char *
lyrics_title(char *str, size_t size)
{
	if (current.loader != NULL) {
		snprintf(str, size, "%s (%s)",
			 _("Lyrics"),
			 /* translators: this message is displayed
			    while data is retrieved */
			 _("loading..."));
		return str;
	} else if (current.artist != NULL && current.title != NULL &&
		   !screen_text_is_empty(&text)) {
		int n;
		n = snprintf(str, size, "%s: %s - %s",
			     _("Lyrics"),
			     current.artist, current.title);
		if (options.lyrics_show_plugin && current.plugin_name != NULL)
			snprintf(str + n, size - n, " (%s)",
				 current.plugin_name);
		return str;
	} else
		return _("Lyrics");
}

static void
lyrics_paint(void)
{
	screen_text_paint(&text);
}

static bool
lyrics_cmd(struct mpdclient *c, command_t cmd)
{
	if (screen_text_cmd(&text, c, cmd))
		return true;

	switch(cmd) {
	case CMD_INTERRUPT:
		if (current.loader != NULL) {
			screen_lyrics_abort();
			screen_text_clear(&text);
		}
		return true;
	case CMD_SAVE_PLAYLIST:
		if (current.loader == NULL && current.artist != NULL &&
		    current.title != NULL && store_lyr_hd() == 0)
			/* lyrics for the song were saved on hard disk */
			screen_status_message (_("Lyrics saved"));
		return true;
	case CMD_DELETE:
		if (current.loader == NULL && current.artist != NULL &&
		    current.title != NULL) {
			switch (delete_lyr_hd()) {
			case 0:
				screen_status_message (_("Lyrics deleted"));
				break;
			case -1:
				screen_status_message (_("No saved lyrics"));
				break;
			}
		}
		return true;
	case CMD_LYRICS_UPDATE:
		if (c->song != NULL) {
			screen_lyrics_load(c->song);
			screen_text_repaint(&text);
		}
		return true;
	case CMD_SELECT:
		if (current.loader == NULL && current.artist != NULL &&
		    current.title != NULL) {
			current.loader = lyrics_load(current.artist, current.title,
						     screen_lyrics_callback, NULL);
			screen_text_repaint(&text);
		}
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (current.song != NULL) {
			screen_song_switch(c, current.song);
			return true;
		}

		break;
#endif
	case CMD_SCREEN_SWAP:
		screen_swap(c, current.song);
		return true;

	case CMD_LOCATE:
		if (current.song != NULL) {
			screen_file_goto_song(c, current.song);
			return true;
		}

		return false;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_lyrics = {
	.init = lyrics_screen_init,
	.exit = lyrics_exit,
	.open = lyrics_open,
	.update = lyrics_update,
	.close = NULL,
	.resize = lyrics_resize,
	.paint = lyrics_paint,
	.cmd = lyrics_cmd,
	.get_title = lyrics_title,
};

void
screen_lyrics_switch(struct mpdclient *c, const struct mpd_song *song, bool f)
{
	assert(song != NULL);

	follow = f;
	next_song = mpd_song_dup(song);
	screen_switch(&screen_lyrics, c);
}
