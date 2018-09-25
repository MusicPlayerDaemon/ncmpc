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

#include "DelayedSeek.hxx"
#include "mpdclient.hxx"

#include <glib.h>

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

	if (c.song != nullptr && (unsigned)id == mpd_song_get_id(c.song))
		if (!mpd_run_seek_id(connection, id, time))
			c.HandleError();

	id = -1;
}

void
DelayedSeek::Cancel() noexcept
{
	if (source_id != 0) {
		g_source_remove(source_id);
		source_id = 0;
	}
}

void
DelayedSeek::OnTimer() noexcept
{
	source_id = 0;
	Commit();
}

/**
 * This timer is invoked after seeking when the user hasn't typed a
 * key for 500ms.  It is used to do the real seeking.
 */
static gboolean
seek_timer(gpointer data)
{
	auto &ds = *(DelayedSeek *)data;
	ds.OnTimer();
	return false;
}

void
DelayedSeek::ScheduleTimer() noexcept
{
	assert(source_id == 0);

	source_id = g_timeout_add(500, seek_timer, this);
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
