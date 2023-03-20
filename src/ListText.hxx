// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef LIST_TEXT_HXX
#define LIST_TEXT_HXX

#include <stddef.h>

class ListText {
public:
	/**
	 * @return the text in the locale charset
	 */
	[[gnu::pure]]
	virtual const char *GetListItemText(char *buffer, size_t size,
					    unsigned i) const noexcept = 0;
};

#endif
