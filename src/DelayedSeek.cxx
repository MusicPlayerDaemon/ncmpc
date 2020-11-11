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
