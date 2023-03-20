// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "DelayedSeek.hxx"
#include "mpdclient.hxx"

void
DelayedSeek::Commit() noexcept
{
	if (id < 0)
		return;

	struct mpd_connection *connection = c.GetConnection();
	if (connection == nullptr) {
		id = -1;
		return;
	}

	if (id == c.GetCurrentSongId())
		if (!mpd_run_seek_id(connection, id, time))
			c.HandleError();

	id = -1;
}

void
DelayedSeek::Cancel() noexcept
{
	commit_timer.Cancel();
}

void
DelayedSeek::OnTimer() noexcept
{
	Commit();
}

void
DelayedSeek::ScheduleTimer() noexcept
{
	commit_timer.Schedule(std::chrono::milliseconds(500));
}

bool
DelayedSeek::Seek(int offset) noexcept
{
	if (!c.playing_or_paused)
		return false;

	int current_id = mpd_status_get_song_id(c.status);
	if (current_id < 0)
		return false;

	int new_time;
	if (current_id == id) {
		new_time = time;
	} else {
		id = current_id;
		new_time = mpd_status_get_elapsed_time(c.status);
	}

	new_time += offset;
	if (new_time < 0)
		new_time = 0;
	else if ((unsigned)new_time > mpd_status_get_total_time(c.status))
		new_time = mpd_status_get_total_time(c.status);

	time = new_time;

	ScheduleTimer();

	return true;
}
