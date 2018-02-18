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

#include "screen_song.hxx"
#include "screen_interface.hxx"
#include "screen_file.hxx"
#include "screen_lyrics.hxx"
#include "screen_find.hxx"
#include "i18n.h"
#include "screen.hxx"
#include "charset.hxx"
#include "time_format.hxx"
#include "mpdclient.hxx"

#include <mpd/client.h>

#include <glib/gprintf.h>
#include <assert.h>
#include <string.h>

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

static const struct tag_label tag_labels[] = {
	{ MPD_TAG_ARTIST, N_("Artist") },
	{ MPD_TAG_TITLE, N_("Title") },
	{ MPD_TAG_ALBUM, N_("Album") },
	{ LABEL_LENGTH, N_("Length") },
	{ LABEL_POSITION, N_("Position") },
	{ MPD_TAG_COMPOSER, N_("Composer") },
	{ MPD_TAG_NAME, N_("Name") },
	{ MPD_TAG_DISC, N_("Disc") },
	{ MPD_TAG_TRACK, N_("Track") },
	{ MPD_TAG_DATE, N_("Date") },
	{ MPD_TAG_GENRE, N_("Genre") },
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

static const char *const stats_labels[] = {
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

class SongPage final : public Page {
	ListWindow lw;

	mpd_song *selected_song;
	mpd_song *played_song;
	GPtrArray *lines;

public:
	SongPage(WINDOW *w, unsigned cols, unsigned rows)
		:lw(w, cols, rows),
		 /* We will need at least 10 lines, so this saves 10 reallocations :) */
		 lines(g_ptr_array_sized_new(10)) {
		lw.hide_cursor = true;
	}

	~SongPage() override {
		Clear();

		g_ptr_array_free(lines, true);
	}

private:
	void Clear();

	/**
	 * Appends a line with a fixed width for the label column.
	 * Handles nullptr strings gracefully.
	 */
	void AppendLine(const char *label, const char *value,
			unsigned label_col);

	void AppendTag(const struct mpd_song *song, enum mpd_tag_type tag);
	void AddSong(const struct mpd_song *song);
	void AppendStatsLine(enum stats_label label, const char *value);
	bool AddStats(struct mpd_connection *connection);

public:
	/* virtual methods from class Page */
	void OnResize(unsigned cols, unsigned rows) override;
	void Paint() const override;
	void Update(struct mpdclient &c) override;
	bool OnCommand(struct mpdclient &c, command_t cmd) override;
	const char *GetTitle(char *s, size_t size) const override;
};

void
SongPage::Clear()
{
	for (guint i = 0; i < lines->len; ++i)
		g_free(g_ptr_array_index(lines, i));

	g_ptr_array_set_size(lines, 0);

	if (selected_song != nullptr) {
		mpd_song_free(selected_song);
		selected_song = nullptr;
	}
	if (played_song != nullptr) {
		mpd_song_free(played_song);
		played_song = nullptr;
	}
}

static const char *
screen_song_list_callback(unsigned idx, void *data)
{
	GPtrArray *lines = (GPtrArray *)data;

	return (const char *)g_ptr_array_index(lines, idx);
}

static Page *
screen_song_init(WINDOW *w, unsigned cols, unsigned rows)
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i) {
		unsigned width = utf8_width(_(tag_labels[i].label));
		if (width > max_tag_label_width)
			max_tag_label_width = width;
	}

	for (unsigned i = 0; i < G_N_ELEMENTS(stats_labels); ++i) {
		if (stats_labels[i] != nullptr) {
			unsigned width = utf8_width(_(stats_labels[i]));

			if (width > max_stats_label_width)
				max_stats_label_width = width;
		}
	}

	return new SongPage(w, cols, rows);
}

void
SongPage::OnResize(unsigned cols, unsigned rows)
{
	list_window_resize(&lw, cols, rows);
}

const char *
SongPage::GetTitle(gcc_unused char *str, gcc_unused size_t size) const
{
	return _("Song viewer");
}

void
SongPage::Paint() const
{
	list_window_paint(&lw, screen_song_list_callback, lines);
}

void
SongPage::AppendLine(const char *label, const char *value, unsigned label_col)
{
	const unsigned label_width = locale_width(label) + 2;

	assert(label != nullptr);
	assert(value != nullptr);
	assert(g_utf8_validate(value, -1, nullptr));

	/* +2 for ': ' */
	label_col += 2;
	const int value_col = lw.cols - label_col;
	/* calculate the number of required linebreaks */
	const gchar *value_iter = value;
	const int label_size = strlen(label) + label_col;

	while (*value_iter != 0) {
		char *entry = (char *)g_malloc(label_size), *entry_iter;
		if (value_iter == value) {
			entry_iter = entry + g_sprintf(entry, "%s: ", label);
			/* fill the label column with whitespaces */
			memset(entry_iter, ' ', label_col - label_width);
			entry_iter += label_col - label_width;
		}
		else {
			/* fill the label column with whitespaces */
			memset(entry, ' ', label_col);
			entry_iter = entry + label_col;
		}
		/* skip whitespaces */
		while (g_ascii_isspace(*value_iter)) ++value_iter;

		char *p = g_strdup(value_iter);
		unsigned width = utf8_cut_width(p, value_col);
		if (width == 0) {
			/* not enough room for anything - bail out */
			g_free(entry);
			g_free(p);
			break;
		}

		*entry_iter = 0;

		value_iter += strlen(p);
		p = replace_utf8_to_locale(p);
		char *q = g_strconcat(entry, p, nullptr);
		g_free(entry);
		g_free(p);

		g_ptr_array_add(lines, q);
	}
}

gcc_pure
static const char *
get_tag_label(unsigned tag)
{
	for (unsigned i = 0; tag_labels[i].label != nullptr; ++i)
		if (tag_labels[i].tag_type == tag)
			return _(tag_labels[i].label);

	assert(tag < MPD_TAG_COUNT);
	return mpd_tag_name((enum mpd_tag_type)tag);
}

void
SongPage::AppendTag(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *label = get_tag_label(tag);
	unsigned i = 0;
	const char *value;

	assert((unsigned)tag < G_N_ELEMENTS(tag_labels));
	assert(label != nullptr);

	while ((value = mpd_song_get_tag(song, tag, i++)) != nullptr)
		AppendLine(label, value, max_tag_label_width);
}

void
SongPage::AddSong(const struct mpd_song *song)
{
	assert(song != nullptr);

	char songpos[16];
	g_snprintf(songpos, sizeof(songpos), "%d", mpd_song_get_pos(song) + 1);
	AppendLine(get_tag_label(LABEL_POSITION), songpos,
		   max_tag_label_width);

	AppendTag(song, MPD_TAG_ARTIST);
	AppendTag(song, MPD_TAG_TITLE);
	AppendTag(song, MPD_TAG_ALBUM);

	/* create time string and add it */
	if (mpd_song_get_duration(song) > 0) {
		char length[16];
		format_duration_short(length, sizeof(length),
				      mpd_song_get_duration(song));

		const char *value = length;

		char buffer[64];

		if (mpd_song_get_end(song) > 0) {
			char start[16], end[16];
			format_duration_short(start, sizeof(start),
					      mpd_song_get_start(song));
			format_duration_short(end, sizeof(end),
					      mpd_song_get_end(song));

			snprintf(buffer, sizeof(buffer), "%s [%s-%s]\n",
				 length, start, end);
			value = buffer;
		} else if (mpd_song_get_start(song) > 0) {
			char start[16];
			format_duration_short(start, sizeof(start),
					      mpd_song_get_start(song));

			snprintf(buffer, sizeof(buffer), "%s [%s-]\n",
				 length, start);
			value = buffer;
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
	AppendTag(song, MPD_TAG_COMMENT);

	AppendLine(get_tag_label(LABEL_PATH), mpd_song_get_uri(song),
		   max_tag_label_width);
}

void
SongPage::AppendStatsLine(enum stats_label label, const char *value)
{
	AppendLine(_(stats_labels[label]), value,
			   max_stats_label_width);
}

bool
SongPage::AddStats(struct mpd_connection *connection)
{
	struct mpd_stats *mpd_stats = mpd_run_stats(connection);
	if (mpd_stats == nullptr)
		return false;

	g_ptr_array_add(lines, g_strdup(_("MPD statistics")) );

	char buf[64];
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_artists(mpd_stats));
	AppendStatsLine(STATS_ARTISTS, buf);
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_albums(mpd_stats));
	AppendStatsLine(STATS_ALBUMS, buf);
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_songs(mpd_stats));
	AppendStatsLine(STATS_SONGS, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_db_play_time(mpd_stats));
	AppendStatsLine(STATS_DBPLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_play_time(mpd_stats));
	AppendStatsLine(STATS_PLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_uptime(mpd_stats));
	AppendStatsLine(STATS_UPTIME, buf);

	GDate *date = g_date_new();
	g_date_set_time_t(date, mpd_stats_get_db_update_time(mpd_stats));
	g_date_strftime(buf, sizeof(buf), "%x", date);
	AppendStatsLine(STATS_DBUPTIME, buf);
	g_date_free(date);

	mpd_stats_free(mpd_stats);
	return true;
}

static void
audio_format_to_string(char *buffer, size_t size,
		       const struct mpd_audio_format *format)
{
#if LIBMPDCLIENT_CHECK_VERSION(2,10,0)
	if (format->bits == MPD_SAMPLE_FORMAT_FLOAT) {
		g_snprintf(buffer, size, "%u:f:%u",
			   format->sample_rate,
			   format->channels);
		return;
	}

	if (format->bits == MPD_SAMPLE_FORMAT_DSD) {
		if (format->sample_rate > 0 &&
		    format->sample_rate % 44100 == 0) {
			/* use shortcuts such as "dsd64" which implies the
			   sample rate */
			g_snprintf(buffer, size, "dsd%u:%u",
				   format->sample_rate * 8 / 44100,
				   format->channels);
			return;
		}

		g_snprintf(buffer, size, "%u:dsd:%u",
			   format->sample_rate,
			   format->channels);
		return;
	}
#endif

	g_snprintf(buffer, size, "%u:%u:%u",
		   format->sample_rate, format->bits,
		   format->channels);
}

void
SongPage::Update(struct mpdclient &c)
{
	/* Clear all lines */
	for (guint i = 0; i < lines->len; ++i)
		g_free(g_ptr_array_index(lines, i));
	g_ptr_array_set_size(lines, 0);

	/* If a song was selected before the song screen was opened */
	if (next_song != nullptr) {
		assert(selected_song == nullptr);
		selected_song = next_song;
		next_song = nullptr;
	}

	if (selected_song != nullptr &&
	    (c.song == nullptr ||
	     strcmp(mpd_song_get_uri(selected_song),
		    mpd_song_get_uri(c.song)) != 0 ||
	     !mpdclient_is_playing(&c))) {
		g_ptr_array_add(lines, g_strdup(_("Selected song")) );
		AddSong(selected_song);
		g_ptr_array_add(lines, g_strdup("\0"));
	}

	if (c.song != nullptr && mpdclient_is_playing(&c)) {
		if (played_song != nullptr)
			mpd_song_free(played_song);

		played_song = mpd_song_dup(c.song);
		g_ptr_array_add(lines, g_strdup(_("Currently playing song")));
		AddSong(played_song);

		if (mpd_status_get_kbit_rate(c.status) > 0) {
			char buf[16];
			g_snprintf(buf, sizeof(buf), _("%d kbps"),
				   mpd_status_get_kbit_rate(c.status));
			AppendLine(get_tag_label(LABEL_BITRATE), buf,
				   max_tag_label_width);
		}

		const struct mpd_audio_format *format =
			mpd_status_get_audio_format(c.status);
		if (format) {
			char buf[32];
			audio_format_to_string(buf, sizeof(buf), format);
			AppendLine(get_tag_label(LABEL_FORMAT), buf,
				   max_tag_label_width);
		}

		g_ptr_array_add(lines, g_strdup("\0"));
	}

	/* Add some statistics about mpd */
	struct mpd_connection *connection = mpdclient_get_connection(&c);
	if (connection != nullptr && !AddStats(connection))
		mpdclient_handle_error(&c);

	list_window_set_length(&lw, lines->len);
	Paint();
}

bool
SongPage::OnCommand(struct mpdclient &c, command_t cmd)
{
	if (list_window_scroll_cmd(&lw, cmd)) {
		Paint();
		return true;
	}

	switch(cmd) {
	case CMD_LOCATE:
		if (selected_song != nullptr) {
			screen_file_goto_song(&c, selected_song);
			return true;
		}
		if (played_song != nullptr) {
			screen_file_goto_song(&c, played_song);
			return true;
		}

		return false;

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		if (selected_song != nullptr) {
			screen_lyrics_switch(&c, selected_song, false);
			return true;
		}
		if (played_song != nullptr) {
			screen_lyrics_switch(&c, played_song, true);
			return true;
		}
		return false;

#endif

	case CMD_SCREEN_SWAP:
		if (selected_song != nullptr)
			screen.Swap(&c, selected_song);
		else
		// No need to check if this is null - we'd pass null anyway
			screen.Swap(&c, played_song);
		return true;

	default:
		break;
	}

	if (screen_find(&lw, cmd, screen_song_list_callback, lines)) {
		/* center the row */
		list_window_center(&lw, lw.selected);
		Paint();
		return true;
	}

	return false;
}

const struct screen_functions screen_song = {
	.init = screen_song_init,
};

void
screen_song_switch(struct mpdclient *c, const struct mpd_song *song)
{
	assert(song != nullptr);

	next_song = mpd_song_dup(song);
	screen.Switch(screen_song, c);
}
