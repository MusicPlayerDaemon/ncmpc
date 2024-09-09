// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "KeyName.hxx"
#include "i18n.h"
#include "ui/Keys.hxx"
#include "util/CharUtil.hxx"
#include "util/StringCompare.hxx"

#include <stdio.h>
#include <stdlib.h>

std::pair<int, const char *>
ParseKeyName(const char *s) noexcept
{
	if (*s == '\'') {
		if (s[1] == '\\' && s[2] == '\'' && s[3] == '\'')
			/* the single quote can be escaped with a
			   backslash */
			return std::make_pair(s[2], s + 4);

		if (s[1] == '\'' || s[2] != '\'')
			return std::make_pair(-1, s);

		return std::make_pair(s[1], s + 3);
	} else if (*s == 'F') {
		char *end;
		const auto value = strtoul(s + 1, &end, 0);
		if (end == s + 1)
			return std::make_pair(-1, end);

		if (value >= 64)
			return std::make_pair(-1, s);

		return std::make_pair(KEY_F(value), end);
	} else if (auto ctrl = StringAfterPrefixIgnoreCase(s, "ctrl-")) {
		int ch = *ctrl++;
		if (ch >= 0x40 && ch < 0x60)
			return std::make_pair(ch - 0x40, ctrl);

		return std::make_pair(-1, s);
	} else if (auto alt = StringAfterPrefixIgnoreCase(s, "alt-")) {
		int ch = *alt++;
		if (ch >= 0x40 && ch < 0x80)
			return std::make_pair(0xc0 | (ch - 0x40), alt);

		return std::make_pair(-1, s);
	} else {
		char *end;
		const auto value = strtol(s, &end, 0);
		if (end == s)
			return std::make_pair(-1, end);

		return std::make_pair(int(value), end);
	}
}

const char *
GetKeyName(int key) noexcept
{
	static char buf[32];

	for (int i = 0; i < 64; ++i) {
		if (key == KEY_F(i)) {
			snprintf(buf, sizeof(buf), "F%u", i);
			return buf;
		}
	}

	const char *name = keyname(key);
	if (name != nullptr) {
		if (name[0] == '^' && name[1] != 0 && name[2] == 0) {
			/* translate "^X" to "Ctrl-X" */
			snprintf(buf, sizeof(buf), "Ctrl-%c", name[1]);
			return buf;
		}

		if (name[0] == 'M' && name[1] == '-' && name[2] != 0 && name[3] == 0) {
			/* translate "M-X" to "Alt-X" */
			snprintf(buf, sizeof(buf), "Alt-%c", name[2]);
			return buf;
		}
	}

	if (key < 256 && IsAlphaNumericASCII(key))
		snprintf(buf, sizeof(buf), "\'%c\'", key);
	else
		snprintf(buf, sizeof(buf), "%d", key);

	return buf;
}

const char *
GetLocalizedKeyName(int key) noexcept
{
	switch(key) {
	case 0:
		return _("Undefined");
	case ' ':
		return _("Space");
	case KEY_RETURN:
		return _("Enter");
	case KEY_BACKSPACE:
		return _("Backspace");
	case KEY_DC:
		return _("Delete");
	case KEY_UP:
		return _("Up");
	case KEY_DOWN:
		return _("Down");
	case KEY_LEFT:
		return _("Left");
	case KEY_RIGHT:
		return _("Right");
	case KEY_HOME:
		return _("Home");
	case KEY_END:
		return _("End");
	case KEY_NPAGE:
		return _("PageDown");
	case KEY_PPAGE:
		return _("PageUp");
	case KEY_TAB:
		return _("Tab");
	case KEY_BTAB:
		return _("Shift+Tab");
	case KEY_ESCAPE:
		return _("Esc");
	case KEY_IC:
		return _("Insert");
	}

	static char buf[32];

	for (int i = 0; i <= 63; i++) {
		if (key == KEY_F(i)) {
			snprintf(buf, sizeof(buf), "F%d", i );
			return buf;
		}
	}

	const char *name = keyname(key);
	if (name == nullptr) {
		snprintf(buf, sizeof(buf), "0x%03X", key);
		return buf;
	}

	if (name[0] == '^' && name[1] != 0 && name[2] == 0) {
		/* translate "^X" to "Ctrl-X" */
		snprintf(buf, sizeof(buf), _("Ctrl-%c"), name[1]);
		return buf;
	}

	if (name[0] == 'M' && name[1] == '-' && name[2] != 0 && name[3] == 0) {
		/* translate "M-X" to "Alt-X" */
		snprintf(buf, sizeof(buf), _("Alt-%c"), name[2]);
		return buf;
	}

	return name;
}
