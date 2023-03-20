// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef GLOBAL_BINDINGS_HXX
#define GLOBAL_BINDINGS_HXX

struct KeyBindings;

[[gnu::const]]
KeyBindings &
GetGlobalKeyBindings() noexcept;

#endif
