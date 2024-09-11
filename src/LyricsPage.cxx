// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "LyricsPage.hxx"
#include "LyricsCache.hxx"
#include "LyricsLoader.hxx"
#include "PageMeta.hxx"
#include "screen_status.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "Options.hxx"
#include "screen.hxx"
#include "plugin.hxx"
#include "page/TextPage.hxx"
#include "lib/fmt/ToSpan.hxx"
#include "client/mpdclient.hxx"
#include "util/StringAPI.hxx"

#include <string>

#include <assert.h>
#include <errno.h>
#include <spawn.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using std::string_view_literals::operator""sv;

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
	LyricsPage(EventLoop &event_loop,
		   PageContainer &_parent, const Window _window,
		   FindSupport &_find_support) noexcept
		:TextPage(_parent, _window, _find_support),
		 plugin_timeout(event_loop,
				BIND_THIS_METHOD(OnTimeout)) {}

	~LyricsPage() noexcept override {
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

	void Cancel() noexcept;

	void Set(const char *s) noexcept;

	void StartPluginCycle() noexcept;

	void Load(const struct mpd_song &song) noexcept;
	void MaybeLoad(const struct mpd_song &new_song) noexcept;

	void MaybeLoad(const struct mpd_song *new_song) noexcept {
		if (new_song != nullptr)
			MaybeLoad(*new_song);
	}

	void Reload() noexcept;

	bool Save() noexcept;

	/** save current lyrics to a file and run editor on it */
	void Edit() noexcept;

	void OnTimeout() noexcept;

public:
	/* virtual methods from class Page */
	void OnOpen(struct mpdclient &c) noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

	const struct mpd_song *GetSelectedSong() const noexcept override {
		return song;
	}

private:
	/* virtual methods from class PluginResponseHandler */
	void OnPluginSuccess(const char *plugin_name,
			     std::string result) noexcept override;
	void OnPluginError(std::string error) noexcept override;
};

void
LyricsPage::Cancel() noexcept
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
LyricsPage::Save() noexcept
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
LyricsPage::Set(const char *s) noexcept
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
	SchedulePaint();
}

void
LyricsPage::OnPluginError(std::string error) noexcept
{
	plugin_name.clear();

	Set(error.c_str());

	/* translators: no lyrics were found for the song */
	Alert(_("No lyrics"));

	plugin_timeout.Cancel();
	StopPluginCycle();

	/* schedule a full repaint so the page title gets updated */
	SchedulePaint();
}

void
LyricsPage::OnTimeout() noexcept
{
	StopPluginCycle();

	FmtAlert(_("Lyrics timeout occurred after {} seconds"),
		 (int)std::chrono::duration_cast<std::chrono::seconds>(options.lyrics_timeout).count());

	/* schedule a full repaint so the page title gets updated */
	SchedulePaint();
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
LyricsPage::Reload() noexcept
{
	if (plugin_cycle == nullptr && artist != nullptr && title != nullptr) {
		reloading = true;
		StartPluginCycle();
		SchedulePaint();
	}
}

static std::unique_ptr<Page>
lyrics_screen_init(ScreenManager &_screen, const Window window) noexcept
{
	return std::make_unique<LyricsPage>(_screen.GetEventLoop(),
					    _screen, window,
					    _screen.find_support);
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

std::string_view
LyricsPage::GetTitle(std::span<char> buffer) const noexcept
{
	if (plugin_cycle != nullptr) {
		return FmtTruncate(buffer, "{} ({})",
				   _("Lyrics"),
				   /* translators: this message is
				      displayed while data is
				      retrieved */
				   _("loading..."));
	} else if (artist != nullptr && title != nullptr && !IsEmpty()) {
		std::size_t n;
		n = snprintf(buffer.data(), buffer.size(), "%s: %s - %s",
			     _("Lyrics"),
			     artist, title);

		if (options.lyrics_show_plugin && !plugin_name.empty() &&
		    n < buffer.size() - 1)
			n += snprintf(buffer.data() + n, buffer.size() - n, " (%s)", plugin_name.c_str());

		return {buffer.data(), n};
	} else
		return _("Lyrics");
}

void
LyricsPage::Edit() noexcept
{
	const auto path = cache.MakePath(artist, title);
	if (path.empty()) {
		Alert(_("Lyrics cache is unavailable"));
		return;
	}

	if (options.text_editor.empty()) {
		Alert(_("Editor not configured"));
		return;
	}

	const char *editor = options.text_editor.c_str();

	if (!Save())
		return;

	def_prog_mode();
	endwin();

	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);

#ifdef USE_SIGNALFD
	/* unblock all signals which may be blocked for signalfd */
	posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK);
#endif

	char *const argv[] = {
		const_cast<char *>(editor),
		const_cast<char *>(path.c_str()),
		nullptr
	};

	pid_t pid;
	if (posix_spawn(&pid, editor, nullptr, &attr,
			argv, environ) != 0) {
		reset_prog_mode();
		FmtAlert("{} ({})"sv, _("Can't start editor"), strerror(errno));
		return;
	}

	int ret, status;
	do {
		ret = waitpid(pid, &status, 0);
	} while (ret == -1 && errno == EINTR);

	reset_prog_mode();

	/* TODO: hardly portable */
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0)
			/* update to get the changes */
			Reload();
		else if (WEXITSTATUS(status) == 127)
			Alert(_("Can't start editor"));
		else
			FmtAlert("{} ({})",
				 _("Editor exited unexpectedly"),
				 WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		FmtAlert("{} (signal {})"sv,
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
			Alert (_("Lyrics saved"));
		return true;
	case Command::DELETE:
		if (plugin_cycle == nullptr && artist != nullptr &&
		    title != nullptr) {
			Alert(cache.Delete(artist, title)
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
