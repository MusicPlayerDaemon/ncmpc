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
#include "screen.h"
#include "i18n.h"

#define IS_PLAYING(s) (s==MPD_STATE_PLAY)
#define IS_PAUSED(s) (s==MPD_STATE_PAUSE)
#define IS_STOPPED(s) (!(IS_PLAYING(s) | IS_PAUSED(s)))

int seek_id = -1;
int seek_target_time;

static guint seek_source_id;

static void
commit_seek(struct mpdclient *c)
{
	if (seek_id < 0)
		return;

	if (c->song != NULL && (unsigned)seek_id == mpd_song_get_id(c->song))
		mpdclient_cmd_seek(c, seek_id, seek_target_time);

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
	if (c->connection == NULL || c->status == NULL)
		return false;

	cancel_seek_timer();

	switch(cmd) {
		/*
	case CMD_PLAY:
		mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
		break;
		*/
	case CMD_PAUSE:
		mpdclient_cmd_pause(c, !IS_PAUSED(mpd_status_get_state(c->status)));
		break;
	case CMD_STOP:
		mpdclient_cmd_stop(c);
		break;
	case CMD_CROP:
		mpdclient_cmd_crop(c);
		break;
	case CMD_SEEK_FORWARD:
		if (!IS_STOPPED(mpd_status_get_state(c->status))) {
			if (c->song != NULL &&
			    seek_id != (int)mpd_song_get_id(c->song)) {
				seek_id = mpd_song_get_id(c->song);
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
		if (!IS_STOPPED(mpd_status_get_state(c->status)))
			mpdclient_cmd_next(c);
		break;
	case CMD_SEEK_BACKWARD:
		if (!IS_STOPPED(mpd_status_get_state(c->status))) {
			if (seek_id != (int)mpd_song_get_id(c->song)) {
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
		if (!IS_STOPPED(mpd_status_get_state(c->status)))
			mpdclient_cmd_prev(c);
		break;
	case CMD_SHUFFLE:
		if (mpdclient_cmd_shuffle(c) == 0)
			screen_status_message(_("Shuffled playlist"));
		break;
	case CMD_CLEAR:
		if (mpdclient_cmd_clear(c) == 0)
			screen_status_message(_("Cleared playlist"));
		break;
	case CMD_REPEAT:
		mpdclient_cmd_repeat(c, !mpd_status_get_repeat(c->status));
		break;
	case CMD_RANDOM:
		mpdclient_cmd_random(c, !mpd_status_get_random(c->status));
		break;
	case CMD_SINGLE:
		mpdclient_cmd_single(c, !mpd_status_get_single(c->status));
		break;
	case CMD_CONSUME:
		mpdclient_cmd_consume(c, !mpd_status_get_consume(c->status));
		break;
	case CMD_CROSSFADE:
		if (mpd_status_get_crossfade(c->status))
			mpdclient_cmd_crossfade(c, 0);
		else
			mpdclient_cmd_crossfade(c, options.crossfade_time);
		break;
	case CMD_DB_UPDATE:
		if (!mpd_status_get_update_id(c->status)) {
			if( mpdclient_cmd_db_update(c,NULL)==0 )
				screen_status_printf(_("Database update started"));
		} else
			screen_status_printf(_("Database update running..."));
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
