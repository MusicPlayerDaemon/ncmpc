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

static ListWindow *lw;

static struct mpd_song *next_song;

static struct {
	struct mpd_song *selected_song;
	struct mpd_song *played_song;
	GPtrArray *lines;
} current;

static void
screen_song_clear()
{
	for (guint i = 0; i < current.lines->len; ++i)
		g_free(g_ptr_array_index(current.lines, i));

	g_ptr_array_set_size(current.lines, 0);

	if (current.selected_song != nullptr) {
		mpd_song_free(current.selected_song);
		current.selected_song = nullptr;
	}
	if (current.played_song != nullptr) {
		mpd_song_free(current.played_song);
		current.played_song = nullptr;
	}
}

static const char *
screen_song_list_callback(unsigned idx, gcc_unused void *data)
{
	assert(idx < current.lines->len);

	return (const char *)g_ptr_array_index(current.lines, idx);
}

static void
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

	/* We will need at least 10 lines, so this saves 10 reallocations :) */
	current.lines = g_ptr_array_sized_new(10);
	lw = new ListWindow(w, cols, rows);
	lw->hide_cursor = true;
}

static void
screen_song_exit()
{
	delete lw;

	screen_song_clear();

	g_ptr_array_free(current.lines, true);
	current.lines = nullptr;
}

static void
screen_song_resize(unsigned cols, unsigned rows)
{
	list_window_resize(lw, cols, rows);
}

static const char *
screen_song_title(gcc_unused char *str, gcc_unused size_t size)
{
	return _("Song viewer");
}

static void
screen_song_paint()
{
	list_window_paint(lw, screen_song_list_callback, nullptr);
}

/* Appends a line with a fixed width for the label column
 * Handles nullptr strings gracefully */
static void
screen_song_append(const char *label, const char *value, unsigned label_col)
{
	const unsigned label_width = locale_width(label) + 2;

	assert(label != nullptr);
	assert(value != nullptr);
	assert(g_utf8_validate(value, -1, nullptr));

	/* +2 for ': ' */
	label_col += 2;
	const int value_col = lw->cols - label_col;
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

		g_ptr_array_add(current.lines, q);
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

static void
screen_song_append_tag(const struct mpd_song *song, enum mpd_tag_type tag)
{
	const char *label = get_tag_label(tag);
	unsigned i = 0;
	const char *value;

	assert((unsigned)tag < G_N_ELEMENTS(tag_labels));
	assert(label != nullptr);

	while ((value = mpd_song_get_tag(song, tag, i++)) != nullptr)
		screen_song_append(label, value, max_tag_label_width);
}

static void
screen_song_add_song(const struct mpd_song *song)
{
	assert(song != nullptr);

	char songpos[16];
	g_snprintf(songpos, sizeof(songpos), "%d", mpd_song_get_pos(song) + 1);
	screen_song_append(get_tag_label(LABEL_POSITION), songpos,
			   max_tag_label_width);

	screen_song_append_tag(song, MPD_TAG_ARTIST);
	screen_song_append_tag(song, MPD_TAG_TITLE);
	screen_song_append_tag(song, MPD_TAG_ALBUM);

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

		screen_song_append(get_tag_label(LABEL_LENGTH), value,
				   max_tag_label_width);
	}

	screen_song_append_tag(song, MPD_TAG_COMPOSER);
	screen_song_append_tag(song, MPD_TAG_NAME);
	screen_song_append_tag(song, MPD_TAG_DISC);
	screen_song_append_tag(song, MPD_TAG_TRACK);
	screen_song_append_tag(song, MPD_TAG_DATE);
	screen_song_append_tag(song, MPD_TAG_GENRE);
	screen_song_append_tag(song, MPD_TAG_COMMENT);

	screen_song_append(get_tag_label(LABEL_PATH), mpd_song_get_uri(song),
			   max_tag_label_width);
}

static void
screen_song_append_stats(enum stats_label label, const char *value)
{
	screen_song_append(_(stats_labels[label]), value,
			   max_stats_label_width);
}

static bool
screen_song_add_stats(struct mpd_connection *connection)
{
	struct mpd_stats *mpd_stats = mpd_run_stats(connection);
	if (mpd_stats == nullptr)
		return false;

	g_ptr_array_add(current.lines, g_strdup(_("MPD statistics")) );

	char buf[64];
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_artists(mpd_stats));
	screen_song_append_stats(STATS_ARTISTS, buf);
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_albums(mpd_stats));
	screen_song_append_stats(STATS_ALBUMS, buf);
	g_snprintf(buf, sizeof(buf), "%d",
		   mpd_stats_get_number_of_songs(mpd_stats));
	screen_song_append_stats(STATS_SONGS, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_db_play_time(mpd_stats));
	screen_song_append_stats(STATS_DBPLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_play_time(mpd_stats));
	screen_song_append_stats(STATS_PLAYTIME, buf);

	format_duration_long(buf, sizeof(buf),
			     mpd_stats_get_uptime(mpd_stats));
	screen_song_append_stats(STATS_UPTIME, buf);

	GDate *date = g_date_new();
	g_date_set_time_t(date, mpd_stats_get_db_update_time(mpd_stats));
	g_date_strftime(buf, sizeof(buf), "%x", date);
	screen_song_append_stats(STATS_DBUPTIME, buf);
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

static void
screen_song_update(struct mpdclient *c)
{
	/* Clear all lines */
	for (guint i = 0; i < current.lines->len; ++i)
		g_free(g_ptr_array_index(current.lines, i));
	g_ptr_array_set_size(current.lines, 0);

	/* If a song was selected before the song screen was opened */
	if (next_song != nullptr) {
		assert(current.selected_song == nullptr);
		current.selected_song = next_song;
		next_song = nullptr;
	}

	if (current.selected_song != nullptr &&
			(c->song == nullptr ||
			 strcmp(mpd_song_get_uri(current.selected_song),
				mpd_song_get_uri(c->song)) != 0 ||
			 !mpdclient_is_playing(c))) {
		g_ptr_array_add(current.lines, g_strdup(_("Selected song")) );
		screen_song_add_song(current.selected_song);
		g_ptr_array_add(current.lines, g_strdup("\0"));
	}

	if (c->song != nullptr && mpdclient_is_playing(c)) {
		if (current.played_song != nullptr) {
			mpd_song_free(current.played_song);
		}
		current.played_song = mpd_song_dup(c->song);
		g_ptr_array_add(current.lines, g_strdup(_("Currently playing song")));
		screen_song_add_song(current.played_song);

		if (mpd_status_get_kbit_rate(c->status) > 0) {
			char buf[16];
			g_snprintf(buf, sizeof(buf), _("%d kbps"),
				   mpd_status_get_kbit_rate(c->status));
			screen_song_append(get_tag_label(LABEL_BITRATE), buf,
					   max_tag_label_width);
		}

		const struct mpd_audio_format *format =
			mpd_status_get_audio_format(c->status);
		if (format) {
			char buf[32];
			audio_format_to_string(buf, sizeof(buf), format);
			screen_song_append(get_tag_label(LABEL_FORMAT), buf,
					   max_tag_label_width);
		}

		g_ptr_array_add(current.lines, g_strdup("\0"));
	}

	/* Add some statistics about mpd */
	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection != nullptr && !screen_song_add_stats(connection))
		mpdclient_handle_error(c);

	list_window_set_length(lw, current.lines->len);
	screen_song_paint();
}

static bool
screen_song_cmd(struct mpdclient *c, command_t cmd)
{
	if (list_window_scroll_cmd(lw, cmd)) {
		screen_song_paint();
		return true;
	}

	switch(cmd) {
	case CMD_LOCATE:
		if (current.selected_song != nullptr) {
			screen_file_goto_song(c, current.selected_song);
			return true;
		}
		if (current.played_song != nullptr) {
			screen_file_goto_song(c, current.played_song);
			return true;
		}

		return false;

#ifdef ENABLE_LYRICS_SCREEN
	case CMD_SCREEN_LYRICS:
		if (current.selected_song != nullptr) {
			screen_lyrics_switch(c, current.selected_song, false);
			return true;
		}
		if (current.played_song != nullptr) {
			screen_lyrics_switch(c, current.played_song, true);
			return true;
		}
		return false;

#endif

	case CMD_SCREEN_SWAP:
		if (current.selected_song != nullptr)
			screen.Swap(c, current.selected_song);
		else
		// No need to check if this is null - we'd pass null anyway
			screen.Swap(c, current.played_song);
		return true;

	default:
		break;
	}

	if (screen_find(lw, cmd, screen_song_list_callback, nullptr)) {
		/* center the row */
		list_window_center(lw, lw->selected);
		screen_song_paint();
		return true;
	}

	return false;
}

const struct screen_functions screen_song = {
	.init = screen_song_init,
	.exit = screen_song_exit,
	.open = screen_song_update,
	.close = screen_song_clear,
	.resize = screen_song_resize,
	.paint = screen_song_paint,
	.update = screen_song_update,
	.cmd = screen_song_cmd,
#ifdef HAVE_GETMOUSE
	.mouse = nullptr,
#endif
	.get_title = screen_song_title,
};

void
screen_song_switch(struct mpdclient *c, const struct mpd_song *song)
{
	assert(song != nullptr);
	assert(current.selected_song == nullptr);
	assert(current.played_song == nullptr);

	next_song = mpd_song_dup(song);
	screen.Switch(screen_song, c);
}
