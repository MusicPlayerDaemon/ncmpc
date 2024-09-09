// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "ListPage.hxx"
#include "ui/ListText.hxx"

#include <vector>
#include <string>

struct mpdclient;
class FindSupport;

class TextPage : public ListPage, ListText {
	FindSupport &find_support;

protected:
	/**
	 * Strings are UTF-8.
	 */
	std::vector<std::string> lines;

public:
	TextPage(PageContainer &_parent, Window window, Size size,
		 FindSupport &_find_support) noexcept;

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

public:
	/* virtual methods from class Page */
	void Paint() const noexcept override;
	bool OnCommand(struct mpdclient &c, Command cmd) override;

private:
	/* virtual methods from class ListText */
	std::string_view GetListItemText(std::span<char> buffer,
					 unsigned i) const noexcept override;
};
