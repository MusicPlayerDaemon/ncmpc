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

#ifndef NCMPC_INSTANCE_HXX
#define NCMPC_INSTANCE_HXX

#include "mpdclient.hxx"
#include "DelayedSeek.hxx"
#include "screen.hxx"

typedef struct _GMainLoop GMainLoop;

/**
 * A singleton holding global instance variables.
 */
class Instance {
	GMainLoop *main_loop;

	struct mpdclient client;

	DelayedSeek seek;

	ScreenManager screen_manager;

public:
	Instance();
	~Instance();

	Instance(const Instance &) = delete;
	Instance &operator=(const Instance &) = delete;

	auto *GetMainLoop() {
		return main_loop;
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
};

#endif
