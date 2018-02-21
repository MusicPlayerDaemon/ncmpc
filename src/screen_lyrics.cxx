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

static struct mpd_song *next_song;
static bool follow = false;

class LyricsPage final : public TextPage {
	ScreenManager &screen;

	/** Set if the cursor position shall be kept during the next lyrics update. */
	bool reloading = false;

	struct mpd_song *song = nullptr;

	char *artist = nullptr, *title = nullptr, *plugin_name = nullptr;

	PluginCycle *loader = nullptr;

	guint loader_timeout = 0;

public:
	LyricsPage(ScreenManager &_screen, WINDOW *w,
		   unsigned cols, unsigned rows)
		:TextPage(w, cols, rows),
		 screen(_screen) {}

	~LyricsPage() override {
		Cancel();
	}

private:
	void Cancel();

	/**
	 * Repaint and update the screen, if it is currently active.
	 */
	void RepaintIfActive() {
		if (screen.IsVisible(*this))
			Repaint();

		/* XXX repaint the screen title */
	}

	void Set(const char *s);

	void Load(const struct mpd_song *song);
	void Reload();

	bool Save();
	bool Delete();

	/** save current lyrics to a file and run editor on it */
	void Edit();

	static void PluginCallback(std::string &&result, bool success,
				   const char *plugin_name, void *data) {
		auto &p = *(LyricsPage *)data;
		p.PluginCallback(std::move(result), success, plugin_name);
	}

	void PluginCallback(std::string &&result, bool success,
			    const char *plugin_name);

	static gboolean TimeoutCallback(gpointer data) {
		auto &p = *(LyricsPage *)data;
		p.TimeoutCallback();
		return false;
	}

	void TimeoutCallback();

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) override;

	void Update(struct mpdclient &c) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;

	const char *GetTitle(char *, size_t) const override;
};

void
LyricsPage::Cancel()
{
	if (loader != nullptr) {
		plugin_stop(loader);
		loader = nullptr;
	}

	if (loader_timeout != 0) {
		g_source_remove(loader_timeout);
		loader_timeout = 0;
	}

	g_free(plugin_name);
	plugin_name = nullptr;

	g_free(artist);
	artist = nullptr;

	g_free(title);
	title = nullptr;

	if (song != nullptr) {
		mpd_song_free(song);
		song = nullptr;
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

bool
LyricsPage::Save()
{
	FILE *lyr_file = create_lyr_file(artist, title);
	if (lyr_file == nullptr)
		return false;

	for (const auto &i : lines)
		fprintf(lyr_file, "%s\n", i.c_str());

	fclose(lyr_file);
	return true;
}

bool
LyricsPage::Delete()
{
	if (!exists_lyr_file(artist, title))
		return false;

	char path[1024];
	path_lyr_file(path, 1024, artist, title);
	return unlink(path) == 0;
}

void
LyricsPage::Set(const char *s)
{
	if (reloading) {
		unsigned saved_start = lw.start;

		TextPage::Set(s);

		/* restore the cursor and ensure that it's still valid */
		lw.start = saved_start;
		list_window_fetch_cursor(&lw);
	} else {
		TextPage::Set(s);
	}

	reloading = false;

	/* paint new data */

	RepaintIfActive();
}

inline void
LyricsPage::PluginCallback(std::string &&result, const bool success,
			   const char *_plugin_name)
{
	assert(loader != nullptr);

	plugin_name = g_strdup(_plugin_name);

	/* Display result, which may be lyrics or error messages */
	Set(result.c_str());

	if (success == true) {
		if (options.lyrics_autosave &&
		    !exists_lyr_file(artist, title))
			Save();
	} else {
		/* translators: no lyrics were found for the song */
		screen_status_message (_("No lyrics"));
	}

	if (loader_timeout != 0) {
		g_source_remove(loader_timeout);
		loader_timeout = 0;
	}

	plugin_stop(loader);
	loader = nullptr;
}

inline void
LyricsPage::TimeoutCallback()
{
	plugin_stop(loader);
	loader = nullptr;

	screen_status_printf(_("Lyrics timeout occurred after %d seconds"),
			     options.lyrics_timeout);

	loader_timeout = 0;
}

void
LyricsPage::Load(const struct mpd_song *_song)
{
	assert(_song != nullptr);

	Cancel();
	Clear();

	const char *_artist = mpd_song_get_tag(_song, MPD_TAG_ARTIST, 0);
	const char *_title = mpd_song_get_tag(_song, MPD_TAG_TITLE, 0);

	song = mpd_song_dup(_song);
	artist = g_strdup(_artist);
	title = g_strdup(_title);

	loader = lyrics_load(artist, title, PluginCallback, this);

	if (options.lyrics_timeout != 0) {
		loader_timeout = g_timeout_add_seconds(options.lyrics_timeout,
						       TimeoutCallback,
						       this);
	}
}

void
LyricsPage::Reload()
{
	if (loader == nullptr && artist != nullptr && title != nullptr) {
		reloading = true;
		loader = lyrics_load(artist, title, PluginCallback, nullptr);
		Repaint();
	}
}

static Page *
lyrics_screen_init(ScreenManager &_screen, WINDOW *w, unsigned cols, unsigned rows)
{
	return new LyricsPage(_screen, w, cols, rows);
}

void
LyricsPage::OnOpen(struct mpdclient &c)
{
	const struct mpd_song *next_song_c =
		next_song != nullptr ? next_song : c.song;

	if (next_song_c != nullptr &&
	    (song == nullptr ||
	     strcmp(mpd_song_get_uri(next_song_c),
		    mpd_song_get_uri(song)) != 0))
		Load(next_song_c);

	if (next_song != nullptr) {
		mpd_song_free(next_song);
		next_song = nullptr;
	}
}

void
LyricsPage::Update(struct mpdclient &c)
{
	if (!follow)
		return;

	if (c.song != nullptr &&
	    (song == nullptr ||
	     strcmp(mpd_song_get_uri(c.song),
		    mpd_song_get_uri(song)) != 0))
		Load(c.song);
}

const char *
LyricsPage::GetTitle(char *str, size_t size) const
{
	if (loader != nullptr) {
		snprintf(str, size, "%s (%s)",
			 _("Lyrics"),
			 /* translators: this message is displayed
			    while data is retrieved */
			 _("loading..."));
		return str;
	} else if (artist != nullptr && title != nullptr && !IsEmpty()) {
		int n;
		n = snprintf(str, size, "%s: %s - %s",
			     _("Lyrics"),
			     artist, title);

		if (options.lyrics_show_plugin && plugin_name != nullptr &&
		    (unsigned int) n < size - 1)
			snprintf(str + n, size - n, " (%s)", plugin_name);

		return str;
	} else
		return _("Lyrics");
}

void
LyricsPage::Edit()
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

	if (!Save())
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
		path_lyr_file(path, sizeof(path), artist, title);
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
			Reload();
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

bool
LyricsPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (TextPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case CMD_INTERRUPT:
		if (loader != nullptr) {
			Cancel();
			Clear();
		}
		return true;
	case CMD_SAVE_PLAYLIST:
		if (loader == nullptr && artist != nullptr &&
		    title != nullptr && Save())
			/* lyrics for the song were saved on hard disk */
			screen_status_message (_("Lyrics saved"));
		return true;
	case CMD_DELETE:
		if (loader == nullptr && artist != nullptr &&
		    title != nullptr) {
			switch (Delete()) {
			case true:
				screen_status_message (_("Lyrics deleted"));
				break;
			case false:
				screen_status_message (_("No saved lyrics"));
				break;
			}
		}
		return true;
	case CMD_LYRICS_UPDATE:
		if (c.song != nullptr)
			Load(c.song);
		return true;
	case CMD_EDIT:
		Edit();
		return true;
	case CMD_SELECT:
		Reload();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case CMD_SCREEN_SONG:
		if (song != nullptr) {
			screen_song_switch(&c, song);
			return true;
		}

		break;
#endif
	case CMD_SCREEN_SWAP:
		screen.Swap(&c, song);
		return true;

	case CMD_LOCATE:
		if (song != nullptr) {
			screen_file_goto_song(&c, song);
			return true;
		}

		return false;

	default:
		break;
	}

	return false;
}

const struct screen_functions screen_lyrics = {
	"lyrics",
	lyrics_screen_init,
};

void
screen_lyrics_switch(struct mpdclient *c, const struct mpd_song *song, bool f)
{
	assert(song != nullptr);

	follow = f;
	next_song = mpd_song_dup(song);
	screen.Switch(screen_lyrics, c);
}
