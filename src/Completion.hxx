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

#ifndef COMPLETION_HXX
#define COMPLETION_HXX

#include <glib.h>

class Completion {
protected:
	GCompletion *const gcmp;

public:
	Completion();
	~Completion();

	Completion(const Completion &) = delete;
	Completion &operator=(const Completion &) = delete;

	GList *Complete(const gchar *prefix, gchar **new_prefix) {
		return g_completion_complete(gcmp, prefix, new_prefix);
	}

	virtual void Pre(const char *value) = 0;
	virtual void Post(const char *value, GList *list) = 0;
};

#endif
