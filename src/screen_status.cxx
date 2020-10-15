/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "screen_status.hxx"
#include "screen.hxx"
#include "ncmpc.hxx"
#include "util/Exception.hxx"

#include <stdarg.h>

void
screen_status_message(const char *msg) noexcept
{
	screen->status_bar.SetMessage(msg);
}

void
screen_status_printf(const char *format, ...) noexcept
{
	va_list ap;
	va_start(ap,format);
	char msg[256];
	vsnprintf(msg, sizeof(msg), format, ap);
	va_end(ap);
	screen_status_message(msg);
}

void
screen_status_error(std::exception_ptr e) noexcept
{
	screen_status_message(GetFullMessage(std::move(e)).c_str());
}
