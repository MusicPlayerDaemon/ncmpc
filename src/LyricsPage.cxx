/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "LyricsPage.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "FileBrowserPage.hxx"
#include "SongPage.hxx"
#include "i18n.h"
#include "Options.hxx"
#include "mpdclient.hxx"
#include "screen.hxx"
#include "lyrics.hxx"
#include "TextPage.hxx"
#include "screen_utils.hxx"
#include "ncu.hxx"

#include <boost/asio/steady_timer.hpp>

#include <string>

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static struct mpd_song *next_song;
static bool follow = false;

class LyricsPage final : public TextPage {
	/** Set if the cursor position shall be kept during the next lyrics update. */
	bool reloading = false;

	struct mpd_song *song = nullptr;

	/**
	 * These are pointers into the #mpd_song above, and will
	 * become invalid as soon as the mpd_song_free() is called.
	 */
	const char *artist = nullptr, *title = nullptr;

	std::string plugin_name;

	PluginCycle *loader = nullptr;

	boost::asio::steady_timer loader_timeout;

public:
	LyricsPage(ScreenManager &_screen, WINDOW *w, Size size)
		:TextPage(_screen, w, size),
		 loader_timeout(_screen.get_io_service()) {}

	~LyricsPage() override {
		Cancel();
	}

	auto &get_io_service() noexcept {
		return screen.get_io_service();
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

	void Load(const struct mpd_song &song) noexcept;
	void MaybeLoad(const struct mpd_song &new_song) noexcept;

	void MaybeLoad(const struct mpd_song *new_song) noexcept {
		if (new_song != nullptr)
			MaybeLoad(*new_song);
	}

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

	void OnTimeout(const boost::system::error_code &error) noexcept;

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *, size_t) const noexcept override;
};

void
LyricsPage::Cancel()
{
	if (loader != nullptr) {
		plugin_stop(loader);
		loader = nullptr;
	}

	loader_timeout.cancel();

	plugin_name.clear();

	if (song != nullptr) {
		mpd_song_free(song);
		song = nullptr;
		artist = nullptr;
		title = nullptr;
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
		unsigned saved_start = lw.GetOrigin();

		TextPage::Set(s);

		/* restore the cursor and ensure that it's still valid */
		lw.SetOrigin(saved_start);
		lw.FetchCursor();
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

	if (_plugin_name != nullptr)
		plugin_name = _plugin_name;
	else
		plugin_name.clear();

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

	loader_timeout.cancel();

	plugin_stop(loader);
	loader = nullptr;
}

void
LyricsPage::OnTimeout(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

	plugin_stop(loader);
	loader = nullptr;

	screen_status_printf(_("Lyrics timeout occurred after %d seconds"),
			     (int)std::chrono::duration_cast<std::chrono::seconds>(options.lyrics_timeout).count());
}

void
LyricsPage::Load(const struct mpd_song &_song) noexcept
{
	Cancel();
	Clear();

	song = mpd_song_dup(&_song);
	artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

	loader = lyrics_load(get_io_service(),
			     artist, title, PluginCallback, this);

	if (options.lyrics_timeout > std::chrono::steady_clock::duration::zero()) {
		boost::system::error_code error;
		loader_timeout.expires_from_now(options.lyrics_timeout,
						error);
		loader_timeout.async_wait(std::bind(&LyricsPage::OnTimeout, this,
						     std::placeholders::_1));
	}
}

void
LyricsPage::MaybeLoad(const struct mpd_song &new_song) noexcept
{
	if (song == nullptr ||
	    strcmp(mpd_song_get_uri(&new_song),
		   mpd_song_get_uri(song)) != 0)
		Load(new_song);
}

void
LyricsPage::Reload()
{
	if (loader == nullptr && artist != nullptr && title != nullptr) {
		reloading = true;
		loader = lyrics_load(get_io_service(),
				     artist, title, PluginCallback, nullptr);
		Repaint();
	}
}

static std::unique_ptr<Page>
lyrics_screen_init(ScreenManager &_screen, WINDOW *w, Size size)
{
	return std::make_unique<LyricsPage>(_screen, w, size);
}

void
LyricsPage::OnOpen(struct mpdclient &c) noexcept
{
	const struct mpd_song *next_song_c =
		next_song != nullptr ? next_song : c.GetPlayingSong();

	MaybeLoad(next_song_c);

	if (next_song != nullptr) {
		mpd_song_free(next_song);
		next_song = nullptr;
	}
}

void
LyricsPage::Update(struct mpdclient &c, unsigned) noexcept
{
	if (follow)
		MaybeLoad(c.GetPlayingSong());
}

const char *
LyricsPage::GetTitle(char *str, size_t size) const noexcept
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

		if (options.lyrics_show_plugin && !plugin_name.empty() &&
		    (unsigned int) n < size - 1)
			snprintf(str + n, size - n, " (%s)",
				 plugin_name.c_str());

		return str;
	} else
		return _("Lyrics");
}

void
LyricsPage::Edit()
{
	if (options.text_editor.empty()) {
		screen_status_message(_("Editor not configured"));
		return;
	}

	const char *editor = options.text_editor.c_str();
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
		screen_status_printf(("%s (%s)"), _("Can't start editor"), strerror(errno));
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
			screen_status_printf("%s (%d)",
					     _("Editor exited unexpectedly"),
					     WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		screen_status_printf("%s (signal %d)",
				     _("Editor exited unexpectedly"),
				     WTERMSIG(status));
	}
}

bool
LyricsPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (TextPage::OnCommand(c, cmd))
		return true;

	switch(cmd) {
	case Command::INTERRUPT:
		if (loader != nullptr) {
			Cancel();
			Clear();
		}
		return true;
	case Command::SAVE_PLAYLIST:
		if (loader == nullptr && artist != nullptr &&
		    title != nullptr && Save())
			/* lyrics for the song were saved on hard disk */
			screen_status_message (_("Lyrics saved"));
		return true;
	case Command::DELETE:
		if (loader == nullptr && artist != nullptr &&
		    title != nullptr) {
			screen_status_message(Delete()
					      ? _("Lyrics deleted")
					      : _("No saved lyrics"));
		}
		return true;
	case Command::LYRICS_UPDATE:
		if (c.GetPlayingSong() != nullptr)
			Load(*c.GetPlayingSong());
		return true;
	case Command::EDIT:
		Edit();
		return true;
	case Command::SELECT:
		Reload();
		return true;

#ifdef ENABLE_SONG_SCREEN
	case Command::SCREEN_SONG:
		if (song != nullptr) {
			screen_song_switch(screen, c, *song);
			return true;
		}

		break;
#endif
	case Command::SCREEN_SWAP:
		screen.Swap(c, song);
		return true;

	case Command::LOCATE:
		if (song != nullptr) {
			screen_file_goto_song(screen, c, *song);
			return true;
		}

		return false;

	default:
		break;
	}

	return false;
}

const PageMeta screen_lyrics = {
	"lyrics",
	N_("Lyrics"),
	Command::SCREEN_LYRICS,
	lyrics_screen_init,
};

void
screen_lyrics_switch(ScreenManager &_screen, struct mpdclient &c,
		     const struct mpd_song &song, bool f)
{
	follow = f;
	next_song = mpd_song_dup(&song);
	_screen.Switch(screen_lyrics, c);
}
