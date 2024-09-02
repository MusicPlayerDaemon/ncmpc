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

	void Paint() const noexcept;

private:
	bool Calculate() noexcept;
};

#endif
