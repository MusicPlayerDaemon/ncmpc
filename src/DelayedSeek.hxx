// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
