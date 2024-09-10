// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
	if (!c.IsReady() || c.status == nullptr)
		return false;

	seek.Cancel();

	switch(cmd) {
		/*
	case Command::PLAY:
		mpdclient_cmd_play(c, MPD_PLAY_AT_BEGINNING);
		break;
		*/
	case Command::PAUSE:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_pause(connection, c.state != MPD_STATE_PAUSE))
			c.HandleError();
		break;
	case Command::STOP:
		if (auto *connection = c.GetConnection();
		    connection != nullptr && !mpd_run_stop(connection))
			c.HandleError();
		break;
	case Command::CROP:
		mpdclient_cmd_crop(c);
		break;
	case Command::SEEK_FORWARD:
		seek.Seek(options.seek_time);
		break;

	case Command::TRACK_NEXT:
		if (auto *connection = c.GetConnection();
		    connection != nullptr && !mpd_run_next(connection))
			c.HandleError();
		break;
	case Command::SEEK_BACKWARD:
		seek.Seek(-int(options.seek_time));
		break;

	case Command::TRACK_PREVIOUS:
		if (auto *connection = c.GetConnection();
		    connection != nullptr && !mpd_run_previous(connection))
			c.HandleError();
		break;
	case Command::SHUFFLE:
		if (auto *connection = c.GetConnection()) {
			if (mpd_run_shuffle(connection))
				screen_status_message(_("Shuffled queue"));
			else
				c.HandleError();
		}

		break;
	case Command::CLEAR:
		if (c.RunClearQueue())
			screen_status_message(_("Cleared queue"));
		break;
	case Command::REPEAT:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_repeat(connection,
				    !mpd_status_get_repeat(c.status)))
			c.HandleError();
		break;
	case Command::RANDOM:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_random(connection,
				    !mpd_status_get_random(c.status)))
			c.HandleError();
		break;
	case Command::SINGLE:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_single(connection,
				    !mpd_status_get_single(c.status)))
			c.HandleError();
		break;
	case Command::CONSUME:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_consume(connection,
				     !mpd_status_get_consume(c.status)))
			c.HandleError();
		break;
	case Command::CROSSFADE:
		if (auto *connection = c.GetConnection();
		    connection != nullptr &&
		    !mpd_run_crossfade(connection,
				       mpd_status_get_crossfade(c.status) > 0
				       ? 0 : options.crossfade_time))
			c.HandleError();
		break;
	case Command::DB_UPDATE:
		screen_database_update(c, nullptr);
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
