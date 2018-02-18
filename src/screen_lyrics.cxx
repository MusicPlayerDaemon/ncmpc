/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
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

#include "screen_lyrics.hxx"
#include "screen_interface.hxx"
#include "screen_status.hxx"
#include "screen_file.hxx"
#include "screen_song.hxx"
#include "i18n.h"
#include "options.hxx"
#include "mpdclient.hxx"
#include "screen.hxx"
#include "lyrics.hxx"
#include "screen_text.hxx"
#include "screen_utils.hxx"
#include "ncu.hxx"

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>

static struct screen_text text;

static struct mpd_song *next_song;
static bool follow = false;
/** Set if the cursor position shall be kept during the next lyrics update. */
static bool reloading = false;

static struct {
	struct mpd_song *song;

	char *artist, *title, *plugin_name;

	struct plugin_cycle *loader;

	guint loader_timeout;
} current;

static void
screen_lyrics_abort()
{
	if (current.loader != nullptr) {
		plugin_stop(current.loader);
		current.loader = nullptr;
	}

	if (current.loader_timeout != 0) {
		g_source_remove(current.loader_timeout);
		current.loader_timeout = 0;
	}

	if (current.plugin_name != nullptr) {
		g_free(current.plugin_name);
		current.plugin_name = nullptr;
	}

	if (current.artist != nullptr) {
		g_free(current.artist);
		current.artist = nullptr;
	}

	if (current.title != nullptr) {
		g_free(current.title);
		current.title = nullptr;
	}

	if (current.song != nullptr) {
		mpd_song_free(current.song);
		current.song = nullptr;
	}
}

/**
 * Repaint and update the screen, if it is currently active.
 */
static void
lyrics_repaint_if_active()
{
	if (screen.IsVisible(screen_lyrics)) {
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
	path_lyr_file(path, 1024, artist, title);

	struct stat result;
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
store_lyr_hd()
{
	FILE *lyr_file = create_lyr_file(current.artist, current.title);
	if (lyr_file == nullptr)
		return -1;

	for (unsigned i = 0; i < text.lines->len; ++i)
		fprintf(lyr_file, "%s\n",
			(const char*)g_ptr_array_index(text.lines, i));

	fclose(lyr_file);
	return 0;
}

static int
delete_lyr_hd()
{
	if (!exists_lyr_file(current.artist, current.title))
		return -1;

	char path[1024];
	path_lyr_file(path, 1024, current.artist, current.title);
	if (unlink(path) != 0)
		return -2;

	return 0;
}

static void
screen_lyrics_set(const GString *str)
{
	if (reloading) {
		unsigned saved_start = text.lw->start;

		screen_text_set(&text, str->str);

		/* restore the cursor and ensure that it's still valid */
		text.lw->start = saved_start;
		list_window_fetch_cursor(text.lw);
	} else {
		screen_text_set(&text, str->str);
	}

	reloading = false;

	/* paint new data */

	lyrics_repaint_if_active();
}

static void
screen_lyrics_callback(const GString *result, const bool success,
		       const char *plugin_name, gcc_unused void *data)
{
	assert(current.loader != nullptr);

	current.plugin_name = g_strdup(plugin_name);

	/* Display result, which may be lyrics or error messages */
	if (result != nullptr)
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
	current.loader = nullptr;
}

static gboolean
screen_lyrics_timeout_callback(gpointer gcc_unused data)
{
	plugin_stop(current.loader);
	current.loader = nullptr;

	screen_status_printf(_("Lyrics timeout occurred after %d seconds"),
			     options.lyrics_timeout);

	current.loader_timeout = 0;
	return false;
}

static void
screen_lyrics_load(const struct mpd_song *song)
{
	assert(song != nullptr);

	screen_lyrics_abort();
	screen_text_clear(&text);

	const char *artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

	current.song = mpd_song_dup(song);
	current.artist = g_strdup(artist);
	current.title = g_strdup(title);

	current.loader = lyrics_load(current.artist, current.title,
				     screen_lyrics_callback, nullptr);

	if (options.lyrics_timeout != 0) {
		current.loader_timeout =
			g_timeout_add_seconds(options.lyrics_timeout,
					      screen_lyrics_timeout_callback,
					      nullptr);
	}
}

static void
screen_lyrics_reload()
{
	if (current.loader == nullptr && current.artist != nullptr &&
	    current.title != nullptr) {
		reloading = true;
		current.loader = lyrics_load(current.artist, current.title,
					     screen_lyrics_callback, nullptr);
		screen_text_repaint(&text);
	}
}

static void
lyrics_screen_init(WINDOW *w, unsigned cols, unsigned rows)
{
	screen_text_init(&text, w, cols, rows);
}

static void
lyrics_resize(unsigned cols, unsigned rows)
{
	screen_text_resize(&text, cols, rows);
}

static void
lyrics_exit()
{
	screen_lyrics_abort();

	screen_text_deinit(&text);
}

static void
lyrics_open(struct mpdclient *c)
{
	const struct mpd_song *next_song_c =
		next_song != nullptr ? next_song : c->song;

	if (next_song_c != nullptr &&
	    (current.song == nullptr ||
	     strcmp(mpd_song_get_uri(next_song_c),
		    mpd_song_get_uri(current.song)) != 0))
		screen_lyrics_load(next_song_c);

	if (next_song != nullptr) {
		mpd_song_free(next_song);
		next_song = nullptr;
	}
}

static void
lyrics_update(struct mpdclient *c)
{
	if (!follow)
		return;

	if (c->song != nullptr &&
	    (current.song == nullptr ||
	     strcmp(mpd_song_get_uri(c->song),
		    mpd_song_get_uri(current.song)) != 0))
		screen_lyrics_load(c->song);
}

static const char *
lyrics_title(char *str, size_t size)
{
	if (current.loader != nullptr) {
		snprintf(str, size, "%s (%s)",
			 _("Lyrics"),
			 /* translators: this message is displayed
			    while data is retrieved */
			 _("loading..."));
		return str;
	} else if (current.artist != nullptr && current.title != nullptr &&
		   !screen_text_is_empty(&text)) {
		int n;
		n = snprintf(str, size, "%s: %s - %s",
			     _("Lyrics"),
			     current.artist, current.title);

		if (options.lyrics_show_plugin && current.plugin_name != nullptr &&
		    (unsigned int) n < size - 1)
			snprintf(str + n, size - n, " (%s)",
				 current.plugin_name);

		return str;
	} else
		return _("Lyrics");
}

static void
lyrics_paint()
{
	screen_text_paint(&text);
}

/* save current lyrics to a file and run editor on it */
static void
lyrics_edit()
{
	char *editor = options.text_editor;
	if (editor == nullptr) {
		screen_status_message(_("Editor not configured"));
		return;
	}

	if (options.text_editor_ask) {
		const char *prompt =
			_("Do you really want to start an editor and edit these lyrics?");
		bool really = screen_get_yesno(prompt, false);
		if (!really) {
			screen_status_message(_("Aborted"));
			return;
		}
	}

	if (store_lyr_hd() < 0)
		return;

	ncu_deinit();

	/* TODO: fork/exec/wait won't work on Windows, but building a command
	   string for system() is too tricky */
	int status;
	pid_t pid = fork();
	if (pid == -1) {
		screen_status_printf(("%s (%s)"), _("Can't start editor"), g_strerror(errno));
		ncu_init();
		return;
	} else if (pid == 0) {
		char path[1024];
		path_lyr_file(path, sizeof(path), current.artist, current.title);
		execlp(editor, editor, path, nullptr);
		/* exec failed, do what system does */
		_exit(127);
	} else {
		int ret;
		do {
			ret = waitpid(pid, &status, 0);
		} while (ret == -1 && errno == EINTR);
	}

	ncu_init();

	/* TODO: hardly portable */
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0)
			/* update to get the changes */
			screen_lyrics_reload();
		else if (WEXITSTATUS(status) == 127)
			screen_status_message(_("Can't start editor"));
		else
			screen_status_printf(_("Editor exited unexpectedly (%d)"),
					     WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		screen_status_printf(_("Editor exited unexpectedly (signal %d)"),
				     WTERMSIG(status));
	}
}

static bool
lyrics_cmd(struct mpdclient *c, command_t cmd)
{
	if (screen_text_cmd(&text, c, cmd))
		return true;

	switch(cmd) {
	case CMD_INTERRUPT:
		if (current.loader != nullptr) {
			screen_lyrics_abort();
			screen_text_clear(&text);
		}
		return true;
	case CMD_SAVE_PLAYLIST:
		if (current.loader == nullptr && current.artist != nullptr &&
		    current.title != nullptr && store_lyr_hd() == 0)
			/* lyrics for the song were saved on hard disk */
			screen_status_message (_("Lyrics saved"));
		return true;
	case CMD_DELETE:
		if (current.loader == nullptr && current.artist != nullptr &&
		    current.title != nullptr) {
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
		if (c->song != nullptr) {
			screen_lyrics_load(c->song);
			screen_text_paint(&text);
		}
		return true;
	case CMD_EDIT:
		lyrics_edit();
		return true;
	case CMD_SELECT:
		screen_lyrics_reload();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (current.song != nullptr) {
			screen_song_switch(c, current.song);
			return true;
		}

		break;
#endif
	case CMD_SCREEN_SWAP:
		screen.Swap(c, current.song);
		return true;

	case CMD_LOCATE:
		if (current.song != nullptr) {
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
	.close = nullptr,
	.resize = lyrics_resize,
	.paint = lyrics_paint,
	.update = lyrics_update,
	.cmd = lyrics_cmd,
#ifdef HAVE_GETMOUSE
	.mouse = nullptr,
#endif
	.get_title = lyrics_title,
};

void
screen_lyrics_switch(struct mpdclient *c, const struct mpd_song *song, bool f)
{
	assert(song != nullptr);

	follow = f;
	next_song = mpd_song_dup(song);
	screen.Switch(screen_lyrics, c);
}
