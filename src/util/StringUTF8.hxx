// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#pragma once

#include "config.h"

class ScopeInitUTF8 {
#ifdef HAVE_LOCALE_T
public:
	ScopeInitUTF8() noexcept;
	~ScopeInitUTF8() noexcept;

	ScopeInitUTF8(const ScopeInitUTF8 &) = delete;
	ScopeInitUTF8 &operator=(const ScopeInitUTF8 &) = delete;
#endif
};

[[gnu::pure]]
int
CollateUTF8(const char *a, const char *b) noexcept;
