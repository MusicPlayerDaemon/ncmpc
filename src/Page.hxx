// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "ui/Point.hxx"
#include "ui/Size.hxx"

#include <curses.h>

#include <utility>
#include <span>
#include <string_view>

enum class Command : unsigned;
struct mpdclient;
struct Window;

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
		SetDirty();
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

	/**
	 * Give this object a chance to override painting the status bar.
	 *
	 * @return true if the status bar was painted, false if this
	 * object is not interested in overriding the status bar
	 * contents
	 */
	virtual bool PaintStatusBarOverride(Window window) const noexcept;

	virtual void Update(struct mpdclient &, unsigned) noexcept {}

	/**
	 * Handle a command.
	 *
	 * Exceptions thrown by this method will be caught and
	 * displayed on the status line.
	 *
	 * @returns true if the command should not be handled by the
	 * ncmpc core
	 */
	virtual bool OnCommand(struct mpdclient &c, Command cmd) = 0;

#ifdef HAVE_GETMOUSE
	/**
	 * Handle a mouse event.
	 *
	 * Exceptions thrown by this method will be caught and
	 * displayed on the status line.
	 *
	 * @return true if the event was handled (and should not be
	 * handled by the ncmpc core)
	 */
	virtual bool OnMouse([[maybe_unused]] struct mpdclient &c,
			     [[maybe_unused]] Point position,
			     [[maybe_unused]] mmask_t bstate) {
		return false;
	}
#endif

	[[gnu::pure]]
	virtual std::string_view GetTitle(std::span<char> buffer) const noexcept = 0;
};
