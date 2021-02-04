/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#ifndef NCMPC_DELAYED_SEEK_HXX
#define NCMPC_DELAYED_SEEK_HXX

#include "event/CoarseTimerEvent.hxx"

struct mpdclient;

/**
 * Helper class which handles user seek commands; it will delay
 * actually sending the seek command to MPD.
 */
class DelayedSeek {
	struct mpdclient &c;

	int id = -1;
	unsigned time;

	CoarseTimerEvent commit_timer;

public:
	DelayedSeek(EventLoop &event_loop,
		    struct mpdclient &_c) noexcept
		:c(_c), commit_timer(event_loop, BIND_THIS_METHOD(OnTimer)) {}

	~DelayedSeek() noexcept {
		Cancel();
	}

	bool IsSeeking(int _id) const noexcept {
		return id >= 0 && _id == id;
	}

	unsigned GetTime() const noexcept {
		return time;
	}

	bool Seek(int offset) noexcept;

	void Commit() noexcept;
	void Cancel() noexcept;

private:
	void OnTimer() noexcept;
	void ScheduleTimer() noexcept;
};

#endif
