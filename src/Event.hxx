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

#ifndef EVENT_HXX
#define EVENT_HXX

#include <glib.h>

#include <chrono>

template<typename T, bool (T::*method)()>
struct BindTimeoutCallback {
	static gboolean Callback(gpointer data) {
		auto &t = *static_cast<T *>(data);
		return (t.*method)();
	}
};

template<typename T, bool (T::*method)()>
inline unsigned
ScheduleTimeout(std::chrono::seconds s, T &t)
{
	return g_timeout_add_seconds(s.count(),
				     BindTimeoutCallback<T, method>::Callback,
				     &t);
}

#endif
