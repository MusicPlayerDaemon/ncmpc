// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_STATUS_BAR_HXX
#define NCMPC_STATUS_BAR_HXX

#include "config.h" // IWYU pragma: keep
#include "Window.hxx"
#include "event/CoarseTimerEvent.hxx"

#ifndef NCMPC_MINI
#include "hscroll.hxx"
#endif

#include <string>

struct mpd_status;
struct mpd_song;
class DelayedSeek;

class StatusBar {
	Window window;

	std::string message;
	CoarseTimerEvent message_timer;

#ifndef NCMPC_MINI
	class hscroll hscroll;
#endif

	const char *left_text;
	char right_text[64];

	std::string center_text;

	unsigned left_width, right_width;

public:
	StatusBar(EventLoop &event_loop,
		  Point p, unsigned width) noexcept;
	~StatusBar() noexcept;

	const Window &GetWindow() const noexcept {
		return window;
	}

	void SetMessage(const char *msg) noexcept;
	void ClearMessage() noexcept;

	void OnResize(Point p, unsigned width) noexcept;
	void Update(const struct mpd_status *status,
		    const struct mpd_song *song,
		    const DelayedSeek &seek) noexcept;
	void Paint() const noexcept;

private:
	void OnMessageTimer() noexcept {
		ClearMessage();
	}
};

#endif
