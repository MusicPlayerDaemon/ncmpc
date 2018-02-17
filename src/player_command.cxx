/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#include "player_command.hxx"
#include "mpdclient.hxx"
#include "options.hxx"
#include "i18n.h"
#include "screen_client.hxx"
#include "screen_status.hxx"

int seek_id = -1;
int seek_target_time;

static guint seek_source_id;

static void
commit_seek(struct mpdclient *c)
{
	if (seek_id < 0)
		return;

	struct mpd_connection *connection = mpdclient_get_connection(c);
	if (connection == nullptr) {
		seek_id = -1;
		return;
	}

	if (c->song != nullptr && (unsigned)seek_id == mpd_song_get_id(c->song))
		if (!mpd_run_seek_id(connection, seek_id, seek_target_time))
			mpdclient_handle_error(c);

	seek_id = -1;
}

/**
 * This timer is invoked after seeking when the user hasn't typed a
 * key for 500ms.  It is used to do the real seeking.
 */
static gboolean
seek_timer(gpointer data)
{
	auto *c = (struct mpdclient *)data;

	seek_source_id = 0;
	commit_seek(c);
	return false;
}

static void
schedule_seek_timer(struct mpdclient *c)
{
	assert(seek_source_id == 0);

	seek_source_id = g_timeout_add(500, seek_timer, c);
}

void
cancel_seek_timer()
{
	if (seek_source_id != 0) {
		g_source_remove(seek_source_id);
		seek_source_id = 0;
	}
}

static bool
setup_seek(struct mpdclient *c)
{
	if (!mpdclient_is_playing(c))
		return false;

	if (seek_id != (int)mpd_status_get_song_id(c->status)) {
		seek_id = mpd_status_get_song_id(c->status);
		seek_target_time = mpd_status_get_elapsed_time(c->status);
	}

	schedule_seek_timer(c);

	return true;
}

bool
handle_player_command(struct mpdclient *c, command_t cmd)
{
	if (!mpdclient_is_connected(c) || c->status == nullptr)
		return false;

	cancel_seek_timer();

	switch(cmd) {
		struct mpd_connection *connection;

		/*
	case CMD_PLAY:
		mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
		break;
		*/
	case CMD_PAUSE:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_pause(connection,
				   mpd_status_get_state(c->status) != MPD_STATE_PAUSE))
			mpdclient_handle_error(c);
		break;
	case CMD_STOP:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_stop(connection))
			mpdclient_handle_error(c);
		break;
	case CMD_CROP:
		mpdclient_cmd_crop(c);
		break;
	case CMD_SEEK_FORWARD:
		if (!setup_seek(c))
			break;

		seek_target_time += options.seek_time;
		if (seek_target_time > (int)mpd_status_get_total_time(c->status))
			seek_target_time = mpd_status_get_total_time(c->status);
		break;

	case CMD_TRACK_NEXT:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_next(connection))
			mpdclient_handle_error(c);
		break;
	case CMD_SEEK_BACKWARD:
		if (!setup_seek(c))
			break;

		seek_target_time -= options.seek_time;
		if (seek_target_time < 0)
			seek_target_time = 0;
		break;

	case CMD_TRACK_PREVIOUS:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_previous(connection))
			mpdclient_handle_error(c);
		break;
	case CMD_SHUFFLE:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (mpd_run_shuffle(connection))
			screen_status_message(_("Shuffled queue"));
		else
			mpdclient_handle_error(c);
		break;
	case CMD_CLEAR:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (mpdclient_cmd_clear(c))
			screen_status_message(_("Cleared queue"));
		break;
	case CMD_REPEAT:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_repeat(connection,
				    !mpd_status_get_repeat(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_RANDOM:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_random(connection,
				    !mpd_status_get_random(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_SINGLE:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_single(connection,
				    !mpd_status_get_single(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_CONSUME:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_consume(connection,
				     !mpd_status_get_consume(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_CROSSFADE:
		connection = mpdclient_get_connection(c);
		if (connection == nullptr)
			break;

		if (!mpd_run_crossfade(connection,
				       mpd_status_get_crossfade(c->status) > 0
				       ? 0 : options.crossfade_time))
			mpdclient_handle_error(c);
		break;
	case CMD_DB_UPDATE:
		screen_database_update(c, nullptr);
		break;
	case CMD_VOLUME_UP:
		mpdclient_cmd_volume_up(c);
		break;
	case CMD_VOLUME_DOWN:
		mpdclient_cmd_volume_down(c);
		break;

	default:
		return false;
	}

	return true;
}
