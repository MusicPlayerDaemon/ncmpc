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

#ifndef NCMPC_INSTANCE_HXX
#define NCMPC_INSTANCE_HXX

#include "config.h"
#include "AsyncUserInput.hxx"
#include "mpdclient.hxx"
#include "DelayedSeek.hxx"
#include "screen.hxx"
#include "event/Loop.hxx"
#include "event/TimerEvent.hxx"

#ifdef ENABLE_LIRC
#include "lirc.hxx"
#endif

/**
 * A singleton holding global instance variables.
 */
class Instance {
	EventLoop event_loop;

	struct mpdclient client;

	DelayedSeek seek;

	/**
	 * This timer is installed when the connection to the MPD
	 * server is broken.  It tries to recover by reconnecting
	 * periodically.
	 */
	TimerEvent reconnect_timer;

	TimerEvent update_timer;
	bool pending_update_timer = false;

#ifndef NCMPC_MINI
	TimerEvent check_key_bindings_timer;
#endif

	ScreenManager screen_manager;

#ifdef ENABLE_LIRC
	LircInput lirc_input;
#endif

	AsyncUserInput user_input;

public:
	Instance();
	~Instance();

	Instance(const Instance &) = delete;
	Instance &operator=(const Instance &) = delete;

	auto &GetClient() {
		return client;
	}

	auto &GetSeek() noexcept {
		return seek;
	}

	auto &GetScreenManager() {
		return screen_manager;
	}

	void Quit() noexcept {
		event_loop.Break();
	}

	void UpdateClient() noexcept;

	void Run();

	template<typename D>
	void ScheduleReconnect(const D &expiry_time) {
		reconnect_timer.Schedule(expiry_time);
	}

	void EnableUpdateTimer() noexcept {
		if (!pending_update_timer)
			ScheduleUpdateTimer();
	}

	void DisableUpdateTimer() noexcept {
		if (pending_update_timer) {
			pending_update_timer = false;
			update_timer.Cancel();
		}
	}

#ifndef NCMPC_MINI
	void ScheduleCheckKeyBindings() noexcept {
		check_key_bindings_timer.Schedule(std::chrono::seconds(10));
	}
#endif

private:
#ifndef _WIN32
	void InitSignals();
	void OnSigwinch() noexcept;
#endif

	void OnReconnectTimer() noexcept;

	void OnUpdateTimer() noexcept;

	void ScheduleUpdateTimer() noexcept {
		pending_update_timer = true;
		update_timer.Schedule(std::chrono::milliseconds(500));
	}

#ifndef NCMPC_MINI
	void OnCheckKeyBindings() noexcept;
#endif
};

#endif
