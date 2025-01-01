// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "SongPage.hxx"
#include "PageMeta.hxx"
#include "Command.hxx"
#include "i18n.h"
#include "screen.hxx"
#include "charset.hxx"
#include "time_format.hxx"
#include "page/ListPage.hxx"
#include "ui/ListText.hxx"
#include "ui/TextListRenderer.hxx"
#include "lib/fmt/ToSpan.hxx"
#include "client/mpdclient.hxx"
#include "util/LocaleString.hxx"
#include "util/StringAPI.hxx"
#include "util/StringStrip.hxx"

#include <mpd/client.h>

#include <fmt/format.h>

#include <algorithm>
#include <iterator>
#include <vector>
#include <string>

#include <assert.h>
#include <time.h>

using std::string_view_literals::operator""sv;

enum {
	LABEL_LENGTH = MPD_TAG_COUNT,
	LABEL_PATH,
	LABEL_BITRATE,
	LABEL_FORMAT,
	LABEL_POSITION,
};

struct tag_label {
	unsigned tag_type;
	const char *label;
};

/* note: add all tags mentioned here to #global_tag_whitelist in
   Instance.cxx */
static constexpr struct tag_label tag_labels[] = {
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ MPD_TAG_WORK, N_("Work") },
	{ LABEL_LENGTH, N_("Length") },
	{ LABEL_POSITION, N_("Position") },
	{ MPD_TAG_COMPOSER, N_("Composer") },
	{ MPD_TAG_PERFORMER, N_("Performer") },
	{ MPD_TAG_CONDUCTOR, N_("Conductor") },
	{ MPD_TAG_NAME, N_("Name") },
	{ MPD_TAG_DISC, N_("Disc") },
	{ MPD_TAG_TRACK, N_("Track") },
	{ MPD_TAG_DATE, N_("Date") },
	{ MPD_TAG_GENRE, N_("Genre") },
	{ MPD_TAG_LABEL, N_("Label") },
	{ MPD_TAG_COMMENT, N_("Comment") },
	{ LABEL_PATH, N_("Path") },
	{ LABEL_BITRATE, N_("Bitrate") },
	{ LABEL_FORMAT, N_("Format") },
	{ 0, nullptr }
};

static unsigned max_tag_label_width;

enum stats_label {
	STATS_ARTISTS,
	STATS_ALBUMS,
	STATS_SONGS,
	STATS_UPTIME,
	STATS_DBUPTIME,
	STATS_PLAYTIME,
	STATS_DBPLAYTIME,
};

static constexpr const char *stats_labels[] = {
	N_("Number of artists"),
	N_("Number of albums"),
	N_("Number of songs"),
	N_("Uptime"),
	N_("Most recent db update"),
	N_("Playtime"),
	N_("DB playtime"),
};

static unsigned max_stats_label_width;

static struct mpd_song *next_song;

class SongPage final : public ListPage, ListText {
	ScreenManager &screen;

	mpd_song *selected_song = nullptr;
	mpd_song *played_song = nullptr;

	std::vector<std::string> lines;

public:
	SongPage(ScreenManager &_screen, const Window _window) noexcept
		:ListPage(_screen, _window),
		 screen(_screen) {
		lw.HideCursor();
	}

	~SongPage() noexcept override {
		Clear();
	}

private:
	void Clear() noexcept;

	/**
	 * Appends a line with a fixed width for the label column.
	 * Handles nullptr strings gracefully.
	 */
	void AppendLine(const char *label, std::string_view value,
			unsigned label_col) noexcept;

	void AppendTag(const struct mpd_song *song,
		       enum mpd_tag_type tag) noexcept;
	void AddSong(const struct mpd_song *song) noexcept;
	void AppendStatsLine(enum stats_label label,
			     std::string_view value) noexcept;
	bool AddStats(struct mpd_connection *connection) noexcept;

public:
	/* virtual methods from class Page */
	void OnClose() noexcept override {
		Clear();
		ListPage::OnClose();
	}

	void Paint() const noexcept override;
	void Update(struct mpdclient &c, unsigned events) noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;
	std::string_view GetTitle(std::span<char> buffer) const noexcept override;

	const struct mpd_song *GetSelectedSong() const noexcept override {
		return selected_song != nullptr ? selected_song : played_song;
	}

private:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;
};

void
SongPage::Clear() noexcept
{
	lines.clear();

	if (selected_song != nullptr) {
		mpd_song_free(selected_song);
		selected_song = nullptr;
	}
	if (played_song != nullptr) {
		mpd_song_free(played_song);
		played_song = nullptr;
	}
}

std::string_view
SongPage::GetListItemText(std::span<char>, unsigned idx) const noexcept
{
	return lines[idx];
}

static std::unique_ptr<Page>
screen_song_init(ScreenManager &_screen, const Window window) noexcept
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i) {
		unsigned width = StringWidthMB(my_gettext(tag_labels[i].label));
		if (width > max_tag_label_width)
			max_tag_label_width = width;
	}

	for (unsigned i = 0; i < std::size(stats_labels); ++i) {
		if (stats_labels[i] != nullptr) {
			unsigned width = StringWidthMB(my_gettext(stats_labels[i]));

			if (width > max_stats_label_width)
				max_stats_label_width = width;
		}
	}

	return std::make_unique<SongPage>(_screen, window);
}

std::string_view
SongPage::GetTitle(std::span<char>) const noexcept
{
	return _("Song viewer");
}

void
SongPage::Paint() const noexcept
{
	lw.Paint(TextListRenderer(*this));
}

void
SongPage::AppendLine(const char *label, std::string_view value_utf8,
		     unsigned label_col) noexcept
{
	assert(label != nullptr);

	static constexpr size_t BUFFER_SIZE = 1024;
	if (label_col >= BUFFER_SIZE - 16)
		return;

	/* +2 for ': ' */
	label_col += 2;
	const int value_col = lw.GetWidth() - label_col;
	/* calculate the number of required linebreaks */
	const Utf8ToLocale value_locale(value_utf8);
	const std::string_view value = value_locale;

	std::string_view value_iter = value;

	while (!value_iter.empty()) {
		char buffer[BUFFER_SIZE];
		const char *const buffer_end = buffer + BUFFER_SIZE;

		char *p = buffer;
		size_t n_space = label_col;
		if (value_iter.data() == value.data()) {
			const size_t label_length = std::min(strlen(label),
							     BUFFER_SIZE - 16);
			p = std::copy_n(label, label_length, p);
			*p++ = ':';
			/* fill the label column with whitespaces */
			const unsigned label_width = StringWidthMB(label);
			n_space -= label_width + 1;
		}

		p = std::fill_n(p, n_space, ' ');

		/* skip whitespaces */
		value_iter = StripLeft(value_iter);

		auto truncated_value = TruncateAtWidthMB(value_iter, value_col);
		if (truncated_value.empty())
			/* not enough room for anything - bail out */
			break;

		value_iter = value_iter.substr(truncated_value.size());

		const std::size_t remaining_space = buffer_end - p;
		if (truncated_value.size() > remaining_space)
			truncated_value = truncated_value.substr(0, remaining_space);

		p = std::copy(truncated_value.begin(), truncated_value.end(), p);
		lines.emplace_back(buffer, p);
	}
}

[[gnu::pure]]
static const char *
get_tag_label(unsigned tag) noexcept
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i)
		if (tag_labels[i].tag_type == tag)
			return my_gettext(tag_labels[i].label);

	assert(tag < MPD_TAG_COUNT);
	return mpd_tag_name((enum mpd_tag_type)tag);
}

void
SongPage::AppendTag(const struct mpd_song *song,
		    enum mpd_tag_type tag) noexcept
{
	const char *label = get_tag_label(tag);
	unsigned i = 0;
	const char *value;

	assert(tag < MPD_TAG_COUNT);
	assert(label != nullptr);

	while ((value = mpd_song_get_tag(song, tag, i++)) != nullptr)
		AppendLine(label, value, max_tag_label_width);
}

void
SongPage::AddSong(const struct mpd_song *song) noexcept
{
	assert(song != nullptr);

	AppendLine(get_tag_label(LABEL_POSITION),
		   fmt::format_int{mpd_song_get_pos(song) + 1}.c_str(),
		   max_tag_label_width);

	AppendTag(song, MPD_TAG_ARTIST);
	AppendTag(song, MPD_TAG_TITLE);
	AppendTag(song, MPD_TAG_ALBUM);

	/* create time string and add it */
	if (mpd_song_get_duration(song) > 0) {
		char length_buffer[16];
		const auto length = format_duration_short(length_buffer, mpd_song_get_duration(song));

		std::string_view value = length;

		char buffer[64];

		if (mpd_song_get_end(song) > 0) {
			char start[16], end[16];

			value = FmtTruncate(buffer, "{} [{}-{}]"sv, length,
					    format_duration_short(start, mpd_song_get_start(song)),
					    format_duration_short(end, mpd_song_get_end(song)));
		} else if (mpd_song_get_start(song) > 0) {
			char start[16];

			value = FmtTruncate(buffer, "{} [{}-]"sv, length,
					    format_duration_short(start, mpd_song_get_start(song)));
		}

		AppendLine(get_tag_label(LABEL_LENGTH), value,
				   max_tag_label_width);
	}

	AppendTag(song, MPD_TAG_COMPOSER);
	AppendTag(song, MPD_TAG_NAME);
	AppendTag(song, MPD_TAG_DISC);
	AppendTag(song, MPD_TAG_TRACK);
	AppendTag(song, MPD_TAG_DATE);
	AppendTag(song, MPD_TAG_GENRE);
	AppendTag(song, MPD_TAG_LABEL);
	AppendTag(song, MPD_TAG_COMMENT);

	AppendLine(get_tag_label(LABEL_PATH), mpd_song_get_uri(song),
		   max_tag_label_width);
}

void
SongPage::AppendStatsLine(enum stats_label label, std::string_view value) noexcept
{
	AppendLine(my_gettext(stats_labels[label]), value, max_stats_label_width);
}

bool
SongPage::AddStats(struct mpd_connection *connection) noexcept
{
	struct mpd_stats *mpd_stats = mpd_run_stats(connection);
	if (mpd_stats == nullptr)
		return false;

	lines.emplace_back(_("MPD statistics"));

	AppendStatsLine(STATS_ARTISTS, fmt::format_int{mpd_stats_get_number_of_artists(mpd_stats)}.c_str());
	AppendStatsLine(STATS_ALBUMS, fmt::format_int{mpd_stats_get_number_of_albums(mpd_stats)}.c_str());
	AppendStatsLine(STATS_SONGS, fmt::format_int{mpd_stats_get_number_of_songs(mpd_stats)}.c_str());

	char buf[64];
	format_duration_long(buf, mpd_stats_get_db_play_time(mpd_stats));
	AppendStatsLine(STATS_DBPLAYTIME, buf);

	format_duration_long(buf, mpd_stats_get_play_time(mpd_stats));
	AppendStatsLine(STATS_PLAYTIME, buf);

	format_duration_long(buf, mpd_stats_get_uptime(mpd_stats));
	AppendStatsLine(STATS_UPTIME, buf);

	const time_t t = mpd_stats_get_db_update_time(mpd_stats);
	strftime(buf, sizeof(buf), "%x", localtime(&t));
	AppendStatsLine(STATS_DBUPTIME, buf);

	mpd_stats_free(mpd_stats);
	return true;
}

[[nodiscard]] [[gnu::pure]]
static std::string_view
audio_format_to_string(std::span<char> buffer,
		       const struct mpd_audio_format &format) noexcept
{
	if (format.bits == MPD_SAMPLE_FORMAT_FLOAT) {
		return FmtTruncate(buffer, "{}:f:{}"sv,
				   format.sample_rate,
				   format.channels);
	}

	if (format.bits == MPD_SAMPLE_FORMAT_DSD) {
		if (format.sample_rate > 0 &&
		    format.sample_rate % 44100 == 0) {
			/* use shortcuts such as "dsd64" which implies the
			   sample rate */

			return FmtTruncate(buffer, "dsd{}:{}"sv,
					   format.sample_rate * 8 / 44100,
					   format.channels);
		}

		return FmtTruncate(buffer, "{}:dsd:{}"sv,
				   format.sample_rate,
				   format.channels);
	}

	return FmtTruncate(buffer, "{}:{}:{}"sv,
			   format.sample_rate,
			   format.bits,
			   format.channels);
}

void
SongPage::Update(struct mpdclient &c, unsigned) noexcept
{
	lines.clear();

	/* If a song was selected before the song screen was opened */
	if (next_song != nullptr) {
		assert(selected_song == nullptr);
		selected_song = next_song;
		next_song = nullptr;
	}

	const auto *const playing_song = c.GetPlayingSong();

	if (selected_song != nullptr &&
	    (playing_song == nullptr ||
	     !StringIsEqual(mpd_song_get_uri(selected_song),
			    mpd_song_get_uri(playing_song)))) {
		lines.emplace_back(_("Selected song"));
		AddSong(selected_song);
		lines.emplace_back(std::string());
	}

	if (playing_song) {
		if (played_song != nullptr)
			mpd_song_free(played_song);

		played_song = mpd_song_dup(playing_song);
		lines.emplace_back(_("Currently playing song"));
		AddSong(played_song);

		if (const auto kbit_rate = mpd_status_get_kbit_rate(c.status);
		    kbit_rate > 0) {
			char buf[16];
			AppendLine(get_tag_label(LABEL_BITRATE),
				   FmtTruncate(buf, _("{} kbps"), kbit_rate),
				   max_tag_label_width);
		}

		const struct mpd_audio_format *format =
			mpd_status_get_audio_format(c.status);
		if (format) {
			char buf[32];
			AppendLine(get_tag_label(LABEL_FORMAT),
				   audio_format_to_string(buf, *format),
				   max_tag_label_width);
		}

		lines.emplace_back(std::string());
	}

	/* Add some statistics about mpd */
	auto *connection = c.GetConnection();
	if (connection != nullptr && !AddStats(connection))
		c.HandleError();

	lw.SetLength(lines.size());
	SchedulePaint();
}

bool
SongPage::OnCommand(struct mpdclient &c, Command cmd)
{
	if (ListPage::OnCommand(c, cmd))
		return true;

	switch (cmd) {
	case Command::LIST_FIND:
	case Command::LIST_RFIND:
	case Command::LIST_FIND_NEXT:
	case Command::LIST_RFIND_NEXT:
		CoStart(screen.find_support.Find(lw, *this, cmd));
		return true;

	default:
		break;
	}

	return false;
}

const PageMeta screen_song = {
	"song",
	N_("Song"),
	Command::SCREEN_SONG,
	screen_song_init,
};

void
screen_song_switch(ScreenManager &_screen, struct mpdclient &c,
		   const struct mpd_song &song) noexcept
{
	next_song = mpd_song_dup(&song);
	_screen.Switch(screen_song, c);
}
