// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef NCMPC_PROGRESS_BAR_HXX
#define NCMPC_PROGRESS_BAR_HXX

#include "ui/Window.hxx"

class ProgressBar {
	UniqueWindow window;

	unsigned current = 0, max = 0;

	unsigned width = 0;

public:
	ProgressBar(Point p, unsigned _width) noexcept;

	void OnResize(Point p, unsigned _width) noexcept;

	bool Set(unsigned current, unsigned max) noexcept;

	int ValueAtX(int x) const noexcept {
		if (max == 0 || x < 0)
			return -1;

		const unsigned ux = static_cast<unsigned>(x);
		const unsigned window_width = window.GetWidth();
		return ux * max / window_width;
	}

	void Paint() const noexcept;

private:
	bool Calculate() noexcept;
};

#endif
