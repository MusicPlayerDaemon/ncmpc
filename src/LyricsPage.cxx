// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "LyricsPage.hxx"
#include "LyricsCache.hxx"
#include "LyricsLoader.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "FileBrowserPage.hxx"
#include "SongPage.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "Options.hxx"
#include "mpdclient.hxx"
#include "screen.hxx"
#include "plugin.hxx"
#include "TextPage.hxx"
#include "screen_utils.hxx"
#include "ncu.hxx"
#include "util/StringAPI.hxx"

#include <string>

#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct mpd_song *next_song;
static bool follow = false;

class LyricsPage final : public TextPage, PluginResponseHandler {
	/** Set if the cursor position shall be kept during the next lyrics update. */
	bool reloading = false;

	struct mpd_song *song = nullptr;

	/**
	 * These are pointers into the #mpd_song above, and will
	 * become invalid as soon as the mpd_song_free() is called.
	 */
	const char *artist = nullptr, *title = nullptr;

	LyricsCache cache;

	std::string plugin_name;

	LyricsLoader loader;

	PluginCycle *plugin_cycle = nullptr;

	CoarseTimerEvent plugin_timeout;

public:
	LyricsPage(ScreenManager &_screen, const Window _window, Size size)
		:TextPage(_screen, _window, size),
		 plugin_timeout(_screen.GetEventLoop(),
				BIND_THIS_METHOD(OnTimeout)) {}

	~LyricsPage() override {
		Cancel();
	}

	auto &GetEventLoop() noexcept {
		return plugin_timeout.GetEventLoop();
	}

private:
	void StopPluginCycle() noexcept {
		assert(plugin_cycle != nullptr);

		plugin_stop(*plugin_cycle);
		plugin_cycle = nullptr;
	}

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

	void StartPluginCycle() noexcept;

	void Load(const struct mpd_song &song) noexcept;
	void MaybeLoad(const struct mpd_song &new_song) noexcept;

	void MaybeLoad(const struct mpd_song *new_song) noexcept {
		if (new_song != nullptr)
			MaybeLoad(*new_song);
	}

	void Reload();

	bool Save();

	/** save current lyrics to a file and run editor on it */
	void Edit();

	void OnTimeout() noexcept;

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	const char *GetTitle(char *, size_t) const noexcept override;

private:
	/* virtual methods from class PluginResponseHandler */
	void OnPluginSuccess(const char *plugin_name,
			     std::string result) noexcept override;
	void OnPluginError(std::string error) noexcept override;
};

void
LyricsPage::Cancel()
{
	if (plugin_cycle != nullptr)
		StopPluginCycle();

	plugin_timeout.Cancel();

	plugin_name.clear();

	if (song != nullptr) {
		mpd_song_free(song);
		song = nullptr;
		artist = nullptr;
		title = nullptr;
	}
}

bool
LyricsPage::Save()
{
	FILE *lyr_file = cache.Save(artist, title);
	if (lyr_file == nullptr)
		return false;

	for (const auto &i : lines)
		fprintf(lyr_file, "%s\n", i.c_str());

	fclose(lyr_file);
	return true;
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

void
LyricsPage::OnPluginSuccess(const char *_plugin_name,
			    std::string result) noexcept
{
	plugin_name = _plugin_name;

	Set(result.c_str());

	if (options.lyrics_autosave && cache.IsAvailable() &&
	    !cache.Exists(artist, title))
		Save();

	plugin_timeout.Cancel();

	StopPluginCycle();

	/* schedule a full repaint so the page title gets updated */
	screen.SchedulePaint();
}

void
LyricsPage::OnPluginError(std::string error) noexcept
{
	plugin_name.clear();

	Set(error.c_str());

	/* translators: no lyrics were found for the song */
	screen_status_message(_("No lyrics"));

	plugin_timeout.Cancel();
	StopPluginCycle();

	/* schedule a full repaint so the page title gets updated */
	screen.SchedulePaint();
}

void
LyricsPage::OnTimeout() noexcept
{
	StopPluginCycle();

	screen_status_printf(_("Lyrics timeout occurred after %d seconds"),
			     (int)std::chrono::duration_cast<std::chrono::seconds>(options.lyrics_timeout).count());

	/* schedule a full repaint so the page title gets updated */
	screen.SchedulePaint();
}

void
LyricsPage::StartPluginCycle() noexcept
{
	assert(artist != nullptr);
	assert(title != nullptr);
	assert(plugin_cycle == nullptr);

	plugin_cycle = loader.Load(GetEventLoop(), artist, title, *this);

	if (options.lyrics_timeout > std::chrono::steady_clock::duration::zero())
		plugin_timeout.Schedule(options.lyrics_timeout);
}

void
LyricsPage::Load(const struct mpd_song &_song) noexcept
{
	Cancel();
	Clear();

	song = mpd_song_dup(&_song);
	artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
	title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);

	if (artist == nullptr || title == nullptr) {
		Cancel();
		return;
	}

	if (auto from_cache = cache.Load(artist, title); !from_cache.empty()) {
		/* cached */
		plugin_name = "cache";
		Set(from_cache.c_str());
	} else
		/* not cached - invoke plugins */
		StartPluginCycle();
}

void
LyricsPage::MaybeLoad(const struct mpd_song &new_song) noexcept
{
	if (song == nullptr ||
	    !StringIsEqual(mpd_song_get_uri(&new_song),
			   mpd_song_get_uri(song)))
		Load(new_song);
}

void
LyricsPage::Reload()
{
	if (plugin_cycle == nullptr && artist != nullptr && title != nullptr) {
		reloading = true;
		StartPluginCycle();
		Repaint();
	}
}

static std::unique_ptr<Page>
lyrics_screen_init(ScreenManager &_screen, const Window window, Size size)
{
	return std::make_unique<LyricsPage>(_screen, window, size);
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
	if (plugin_cycle != nullptr) {
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
	const auto path = cache.MakePath(artist, title);
	if (path.empty()) {
		screen_status_message(_("Lyrics cache is unavailable"));
		return;
	}

	if (options.text_editor.empty()) {
		screen_status_message(_("Editor not configured"));
		return;
	}

	const char *editor = options.text_editor.c_str();
	if (options.text_editor_ask) {
		const char *prompt =
			_("Do you really want to start an editor and edit these lyrics?");
		bool really = screen_get_yesno(screen, prompt, false);
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
		execlp(editor, editor, path.c_str(), nullptr);
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
		if (plugin_cycle != nullptr) {
			Cancel();
			Clear();
		}
		return true;
	case Command::SAVE_PLAYLIST:
		if (plugin_cycle == nullptr && artist != nullptr &&
		    title != nullptr && Save())
			/* lyrics for the song were saved on hard disk */
			screen_status_message (_("Lyrics saved"));
		return true;
	case Command::DELETE:
		if (plugin_cycle == nullptr && artist != nullptr &&
		    title != nullptr) {
			screen_status_message(cache.Delete(artist, title)
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
