// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef TEXT_PAGE_HXX
#define TEXT_PAGE_HXX

#include "ListPage.hxx"
#include "ListText.hxx"

#include <vector>
#include <string>

struct mpdclient;
class ScreenManager;

class TextPage : public ListPage, ListText {
protected:
	ScreenManager &screen;

	/**
	 * Strings are UTF-8.
	 */
	std::vector<std::string> lines;

public:
	TextPage(ScreenManager &_screen,
		 WINDOW *w, Size size) noexcept
		:ListPage(w, size), screen(_screen) {
		lw.HideCursor();
	}

protected:
	bool IsEmpty() const noexcept {
		return lines.empty();
	}

	void Clear() noexcept;

	/**
	 * @param str a UTF-8 string
	 */
	void Append(const char *str) noexcept;

	/**
	 * @param str a UTF-8 string
	 */
	void Set(const char *str) noexcept {
		Clear();
		Append(str);
	}

	/**
	 * Repaint and update the screen.
	 */
	void Repaint() noexcept {
		Paint();
		lw.Refresh();
	}

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

private:
	/* virtual methods from class ListText */
	const char *GetListItemText(char *buffer, size_t size,
				    unsigned i) const noexcept override;
};

#endif
