// SPDX-License-Identifier: GPL-2.0-or-lateryes
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"
#include "ui/Point.hxx"
#include "ui/Size.hxx"
#include "co/InvokeTask.hxx"

#include <curses.h>

#include <exception>
#include <utility>
#include <span>
#include <string_view>

enum class Command : unsigned;
struct mpd_song;
struct mpdclient;
struct Window;
class PageContainer;

class Page {
	PageContainer &parent;

	/**
	 * A coroutine running an asynchronous operation.  It will be
	 * canceled when the page is closed.
	 */
	Co::InvokeTask co_task;

	Size last_size{0, 0};

	/**
	 * The MPD idle event mask pending to be submitted to
	 * Update().
	 */
	unsigned pending_events = ~0u;

protected:
	explicit Page(PageContainer &_parent) noexcept
		:parent(_parent) {}

public:
	virtual ~Page() noexcept = default;

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

	void SchedulePaint() noexcept;

	void Alert(std::string message) noexcept;

	/**
	 * Start a coroutine.  This method returns when the coroutine
	 * finishes or gets suspended.  When the coroutine finishes,
	 * OnCoComplete() gets called.
	 *
	 * If suspended, then the coroutine can be canceled using
         * CoCancel().
	 *
	 * If a coroutine is already in progress (but suspended), it
         * is canceled.
	 */
	void CoStart(Co::InvokeTask _task) noexcept;

	/**
	 * Cancel further execution of the coroutine started with
         * CoStart().  After returning, the coroutine promise and
         * stack frame are destructed.
	 *
	 * Calling this method is only allowed if the coroutine is
         * suspended.
	 */
	void CoCancel() noexcept {
		co_task = {};
	}

public:
	virtual void OnOpen(struct mpdclient &) noexcept {}
	virtual void OnClose() noexcept;
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

	/**
	 * Called when the coroutine started with CoStart() finishes.
         * It is not called when the coroutine is canceled.
	 */
	virtual void OnCoComplete() noexcept;

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

	/**
	 * Returns a pointer to the #mpd_song that is currently
         * selected on this page.  The pointed-to object is owned by
         * this #Page instance and may be invalidated at any time.
	 */
	[[gnu::pure]]
	virtual const struct mpd_song *GetSelectedSong() const noexcept {
		return nullptr;
	}

private:
	void _OnCoComplete(std::exception_ptr error) noexcept;
};
