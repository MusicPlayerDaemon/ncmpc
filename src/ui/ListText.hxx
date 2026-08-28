// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include <span>
#include <string_view>

class ListText {
public:
	/**
	 * @return the text in the locale charset
	 */
	[[gnu::pure]]
	virtual std::string_view GetListItemText(std::span<char> buffer,
						 unsigned i) const noexcept = 0;

	/**
	 * Like GetListItemText(), but return a second string (e.g. a
	 * second column).  Only used for searching, not for
	 * rendering.
	 */
	[[gnu::pure]]
	virtual std::string_view GetSecondListItemText([[maybe_unused]] std::span<char> buffer,
						       [[maybe_unused]] unsigned i) const noexcept {
		return {};
	}
};
