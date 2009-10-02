/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "player_command.h"
#include "mpdclient.h"
#include "options.h"
#include "i18n.h"
#include "screen_client.h"
#include "screen_message.h"

int seek_id = -1;
int seek_target_time;

static guint seek_source_id;

static void
commit_seek(struct mpdclient *c)
{
	if (seek_id < 0)
		return;

	if (!mpdclient_is_connected(c)) {
		seek_id = -1;
		return;
	}

	if (c->song != NULL && (unsigned)seek_id == mpd_song_get_id(c->song))
		if (!mpd_run_seek_id(c->connection, seek_id, seek_target_time))
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
	struct mpdclient *c = data;

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
cancel_seek_timer(void)
{
	if (seek_source_id != 0) {
		g_source_remove(seek_source_id);
		seek_source_id = 0;
	}
}

bool
handle_player_command(struct mpdclient *c, command_t cmd)
{
	const struct mpd_song *song;

	if (!mpdclient_is_connected(c) || c->status == NULL)
		return false;

	cancel_seek_timer();

	switch(cmd) {
		/*
	case CMD_PLAY:
		mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
		break;
		*/
	case CMD_PAUSE:
		if (!mpd_run_pause(c->connection,
				   mpd_status_get_state(c->status) != MPD_STATE_PAUSE))
			mpdclient_handle_error(c);
		break;
	case CMD_STOP:
		if (!mpd_run_stop(c->connection))
			mpdclient_handle_error(c);
		break;
	case CMD_CROP:
		mpdclient_cmd_crop(c);
		break;
	case CMD_SEEK_FORWARD:
		song = mpdclient_get_current_song(c);
		if (song != NULL) {
			if (seek_id != (int)mpd_song_get_id(song)) {
				seek_id = mpd_song_get_id(song);
				seek_target_time = mpd_status_get_elapsed_time(c->status);
			}
			seek_target_time+=options.seek_time;
			if (seek_target_time > (int)mpd_status_get_total_time(c->status))
				seek_target_time = mpd_status_get_total_time(c->status);
			schedule_seek_timer(c);
		}
		break;
		/* fall through... */
	case CMD_TRACK_NEXT:
		if (!mpd_run_next(c->connection))
			mpdclient_handle_error(c);
		break;
	case CMD_SEEK_BACKWARD:
		song = mpdclient_get_current_song(c);
		if (song != NULL) {
			if (seek_id != (int)mpd_song_get_id(song)) {
				seek_id = mpd_song_get_id(c->song);
				seek_target_time = mpd_status_get_elapsed_time(c->status);
			}
			seek_target_time-=options.seek_time;
			if (seek_target_time < 0)
				seek_target_time=0;
			schedule_seek_timer(c);
		}
		break;
	case CMD_TRACK_PREVIOUS:
		if (!mpd_run_previous(c->connection))
			mpdclient_handle_error(c);
		break;
	case CMD_SHUFFLE:
		if (mpd_run_shuffle(c->connection))
			screen_status_message(_("Shuffled playlist"));
		else
			mpdclient_handle_error(c);
		break;
	case CMD_CLEAR:
		if (mpdclient_cmd_clear(c))
			screen_status_message(_("Cleared playlist"));
		break;
	case CMD_REPEAT:
		if (!mpd_run_repeat(c->connection,
				    !mpd_status_get_repeat(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_RANDOM:
		if (!mpd_run_random(c->connection,
				    !mpd_status_get_random(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_SINGLE:
		if (!mpd_run_single(c->connection,
				    !mpd_status_get_single(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_CONSUME:
		if (!mpd_run_consume(c->connection,
				     !mpd_status_get_consume(c->status)))
			mpdclient_handle_error(c);
		break;
	case CMD_CROSSFADE:
		if (!mpd_run_crossfade(c->connection,
				       mpd_status_get_crossfade(c->status) > 0
				       ? 0 : options.crossfade_time))
			mpdclient_handle_error(c);
		break;
	case CMD_DB_UPDATE:
		screen_database_update(c, NULL);
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
