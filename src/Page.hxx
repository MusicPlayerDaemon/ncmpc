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

#ifndef NCMPC_PAGE_HXX
#define NCMPC_PAGE_HXX

#include "config.h"
#include "Point.hxx"
#include "Size.hxx"
#include "util/Compiler.h"

#include <curses.h>

#include <utility>

#include <stddef.h>

enum class Command : unsigned;
struct mpdclient;

class Page {
	Size last_size{0, 0};

	/**
	 * The MPD idle event mask pending to be submitted to
	 * Update().
	 */
	unsigned pending_events = ~0u;

	/**
	 * Does this page need to be repainted?
	 */
	bool dirty = true;

public:
	virtual ~Page() noexcept = default;

	bool IsDirty() const noexcept {
		return dirty;
	}

	void SetDirty(bool _dirty=true) noexcept {
		dirty = _dirty;
	}

	void Resize(Size new_size) noexcept {
		if (new_size == last_size)
			return;

		last_size = new_size;
		OnResize(new_size);
	}

	void AddPendingEvents(unsigned events) noexcept {
		pending_events |= events;
	}

	void Update(struct mpdclient &c) noexcept {
		Update(c, std::exchange(pending_events, 0));
	}

protected:
	const Size &GetLastSize() const noexcept {
		return last_size;
	}

public:
	virtual void OnOpen(struct mpdclient &) noexcept {}
	virtual void OnClose() noexcept {}
	virtual void OnResize(Size size) noexcept = 0;
	virtual void Paint() const noexcept = 0;
	virtual void Update(struct mpdclient &, unsigned) noexcept {}

	/**
	 * Handle a command.
	 *
	 * @returns true if the command should not be handled by the
	 * ncmpc core
	 */
	virtual bool OnCommand(struct mpdclient &c, Command cmd) = 0;

#ifdef HAVE_GETMOUSE
	/**
	 * Handle a mouse event.
	 *
	 * @return true if the event was handled (and should not be
	 * handled by the ncmpc core)
	 */
	virtual bool OnMouse(gcc_unused struct mpdclient &c,
			     gcc_unused Point position,
			     gcc_unused mmask_t bstate) {
		return false;
	}
#endif

	gcc_pure
	virtual const char *GetTitle(char *s, size_t size) const noexcept = 0;
};

#endif
