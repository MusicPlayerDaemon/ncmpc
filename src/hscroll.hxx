// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "BasicMarquee.hxx"
#include "event/FineTimerEvent.hxx"
#include "Point.hxx"

#include <curses.h>

enum class Style : unsigned;

/**
 * This class is used to auto-scroll text which does not fit on the
 * screen.  Call hscroll_init() to initialize the object,
 * hscroll_clear() to free resources, and hscroll_set() to begin
 * scrolling.
 */
class hscroll {
	WINDOW *const w;

	BasicMarquee basic;

	/**
	 * The postion on the screen.
	 */
	Point position;

	/**
	 * Style for drawing the text.
	 */
	Style style;

	attr_t attr;

	/**
	 * A timer which updates the scrolled area every second.
	 */
	FineTimerEvent timer;

public:
	hscroll(EventLoop &event_loop,
		WINDOW *_w, const char *_separator) noexcept
		:w(_w), basic(_separator),
		 timer(event_loop, BIND_THIS_METHOD(OnTimer))
	{
	}

	bool IsDefined() const noexcept {
		return basic.IsDefined();
	}

	/**
	 * Sets a text to scroll.  This installs a timer which redraws
	 * every second with the current window attributes.  Call
	 * hscroll_clear() to disable it.
	 */
	void Set(Point _position, unsigned width, const char *text,
		 Style style, attr_t attr=0) noexcept;

	/**
	 * Removes the text and the timer.  It may be reused with
	 * Set().
	 */
	void Clear() noexcept;

	void Rewind() noexcept {
		basic.Rewind();
	}

	void Step() noexcept {
		basic.Step();
	}

	/**
	 * Explicitly draws the scrolled text.  Calling this function
	 * is only allowed if there is a text currently.
	 */
	void Paint() const noexcept;

private:
	void OnTimer() noexcept;

	void ScheduleTimer() noexcept {
		timer.Schedule(std::chrono::seconds(1));
	}
};
