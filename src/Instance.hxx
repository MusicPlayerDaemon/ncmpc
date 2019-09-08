/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#ifndef NCMPC_INSTANCE_HXX
#define NCMPC_INSTANCE_HXX

#include "config.h"
#include "AsyncUserInput.hxx"
#include "mpdclient.hxx"
#include "DelayedSeek.hxx"
#include "screen.hxx"

#ifdef ENABLE_LIRC
#include "lirc.hxx"
#endif

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#ifndef _WIN32
#include <boost/asio/signal_set.hpp>
#endif

/**
 * A singleton holding global instance variables.
 */
class Instance {
	boost::asio::io_service io_service;

#ifndef _WIN32
	boost::asio::signal_set sigterm, sigwinch;
#endif

	struct mpdclient client;

	DelayedSeek seek;

	/**
	 * This timer is installed when the connection to the MPD
	 * server is broken.  It tries to recover by reconnecting
	 * periodically.
	 */
	boost::asio::steady_timer reconnect_timer;

	boost::asio::steady_timer update_timer;
	bool pending_update_timer = false;

#ifndef NCMPC_MINI
	boost::asio::steady_timer check_key_bindings_timer;
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

	auto &get_io_service() {
		return io_service;
	}

	auto &GetClient() {
		return client;
	}

	auto &GetSeek() noexcept {
		return seek;
	}

	auto &GetScreenManager() {
		return screen_manager;
	}

	void UpdateClient() noexcept;

	void Run();

	template<typename D>
	void ScheduleReconnect(const D &expiry_time) {
		boost::system::error_code error;
		reconnect_timer.expires_from_now(expiry_time, error);
		reconnect_timer.async_wait(std::bind(&Instance::OnReconnectTimer,
						     this,
						     std::placeholders::_1));
	}

	void EnableUpdateTimer() noexcept {
		if (!pending_update_timer)
			ScheduleUpdateTimer();
	}

	void DisableUpdateTimer() noexcept {
		if (pending_update_timer) {
			pending_update_timer = false;
			update_timer.cancel();
		}
	}

#ifndef NCMPC_MINI
	void ScheduleCheckKeyBindings() noexcept {
		boost::system::error_code error;
		check_key_bindings_timer.expires_from_now(std::chrono::seconds(10),
							  error);
		check_key_bindings_timer.async_wait(std::bind(&Instance::OnCheckKeyBindings,
							      this,
							      std::placeholders::_1));
	}
#endif

private:
#ifndef _WIN32
	void InitSignals();
	void OnSigwinch();
	void AsyncWaitSigwinch();
#endif

	void OnReconnectTimer(const boost::system::error_code &error) noexcept;

	void OnUpdateTimer(const boost::system::error_code &error) noexcept;

	void ScheduleUpdateTimer() noexcept {
		pending_update_timer = true;
		boost::system::error_code error;
		update_timer.expires_from_now(std::chrono::milliseconds(500),
					      error);
		update_timer.async_wait(std::bind(&Instance::OnUpdateTimer,
						  this,
						  std::placeholders::_1));
	}

#ifndef NCMPC_MINI
	void OnCheckKeyBindings(const boost::system::error_code &error) noexcept;
#endif
};

#endif
