/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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
#include "DelayedSeek.hxx"
#include "Command.hxx"
#include "mpdclient.hxx"
#include "Options.hxx"
#include "i18n.h"
#include "screen_client.hxx"
#include "screen_status.hxx"

bool
handle_player_command(struct mpdclient &c, DelayedSeek &seek, Command cmd)
{
	if (!c.IsConnected() || c.status == nullptr)
		return false;

	seek.Cancel();

	switch(cmd) {
		struct mpd_connection *connection;

		/*
	case Command::PLAY:
		mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
		break;
		*/
	case Command::PAUSE:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_pause(connection, c.state != MPD_STATE_PAUSE))
			c.HandleError();
		break;
	case Command::STOP:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_stop(connection))
			c.HandleError();
		break;
	case Command::CROP:
		mpdclient_cmd_crop(&c);
		break;
	case Command::SEEK_FORWARD:
		seek.Seek(options.seek_time);
		break;

	case Command::TRACK_NEXT:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_next(connection))
			c.HandleError();
		break;
	case Command::SEEK_BACKWARD:
		seek.Seek(-int(options.seek_time));
		break;

	case Command::TRACK_PREVIOUS:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_previous(connection))
			c.HandleError();
		break;
	case Command::SHUFFLE:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (mpd_run_shuffle(connection))
			screen_status_message(_("Shuffled queue"));
		else
			c.HandleError();
		break;
	case Command::CLEAR:
		if (c.RunClearQueue())
			screen_status_message(_("Cleared queue"));
		break;
	case Command::REPEAT:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_repeat(connection,
				    !mpd_status_get_repeat(c.status)))
			c.HandleError();
		break;
	case Command::RANDOM:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_random(connection,
				    !mpd_status_get_random(c.status)))
			c.HandleError();
		break;
	case Command::SINGLE:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_single(connection,
				    !mpd_status_get_single(c.status)))
			c.HandleError();
		break;
	case Command::CONSUME:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_consume(connection,
				     !mpd_status_get_consume(c.status)))
			c.HandleError();
		break;
	case Command::CROSSFADE:
		connection = c.GetConnection();
		if (connection == nullptr)
			break;

		if (!mpd_run_crossfade(connection,
				       mpd_status_get_crossfade(c.status) > 0
				       ? 0 : options.crossfade_time))
			c.HandleError();
		break;
	case Command::DB_UPDATE:
		screen_database_update(&c, nullptr);
		break;
	case Command::VOLUME_UP:
		c.RunVolumeUp();
		break;
	case Command::VOLUME_DOWN:
		c.RunVolumeDown();
		break;

	default:
		return false;
	}

	return true;
}
