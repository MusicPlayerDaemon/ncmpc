/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "screen_utils.hxx"
#include "screen.hxx"
#include "mpdclient.hxx"
#include "config.h"
#include "i18n.h"
#include "options.hxx"
#include "colors.hxx"
#include "wreadln.hxx"
#include "ncmpc.hxx"

#include <mpd/client.h>

#include <string.h>
#include <ctype.h>

void
screen_bell()
{
	if (options.audible_bell)
		beep();
	if (options.visible_bell)
		flash();
}

static bool
ignore_key(int key)
{
	return
#ifdef HAVE_GETMOUSE
		/* ignore mouse events */
		key == KEY_MOUSE ||
#endif
		key == ERR;
}

int
screen_getch(const char *prompt)
{
	WINDOW *w = screen->status_bar.GetWindow().w;

	colors_use(w, COLOR_STATUS_ALERT);
	werase(w);
	wmove(w, 0, 0);
	waddstr(w, prompt);

	echo();
	curs_set(1);

	int key;
	while (ignore_key(key = wgetch(w))) {}

	noecho();
	curs_set(0);

	return key;
}

bool
screen_get_yesno(const char *_prompt, bool def)
{
	/* NOTE: if one day a translator decides to use a multi-byte character
	   for one of the yes/no keys, we'll have to parse it properly */

	char *prompt = g_strdup_printf(_("%s [%s/%s] "), _prompt,
				       YES_TRANSLATION, NO_TRANSLATION);
	int key = tolower(screen_getch(prompt));
	g_free(prompt);

	if (key == YES_TRANSLATION[0])
		return true;
	else if (key == NO_TRANSLATION[0])
		return false;
	else
		return def;
}

std::string
screen_readln(const char *prompt,
	      const char *value,
	      History *history,
	      Completion *completion)
{
	auto *window = &screen->status_bar.GetWindow();
	WINDOW *w = window->w;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);
	auto result = wreadln(w, prompt, value, window->size.width,
			      history, completion);
	curs_set(0);
	return std::move(result);
}

std::string
screen_read_password(const char *prompt)
{
	auto *window = &screen->status_bar.GetWindow();
	WINDOW *w = window->w;

	wmove(w, 0,0);
	curs_set(1);
	colors_use(w, COLOR_STATUS_ALERT);

	if (prompt == nullptr)
		prompt = _("Password");

	auto result = wreadln_masked(w, prompt, nullptr, window->size.width);
	curs_set(0);
	return std::move(result);
}

static const char *
CompletionDisplayString(const char *value)
{
	const char *slash = strrchr(value, '/');
	if (slash == nullptr)
		return value;

	if (slash[1] == 0) {
		/* if the string ends with a slash (directory URIs
		   usually do), backtrack to the preceding slash (if
		   any) */
		while (slash > value) {
			--slash;

			if (*slash == '/')
				return slash + 1;
		}
	}

	return slash;
}

void
screen_display_completion_list(Completion::Range range)
{
	static Completion::Range prev_range;
	static guint prev_length = 0;
	static guint offset = 0;
	WINDOW *w = screen->main_window.w;

	unsigned length = std::distance(range.begin(), range.end());
	if (range == prev_range && length == prev_length) {
		offset += screen->main_window.size.height;
		if (offset >= length)
			offset = 0;
	} else {
		prev_range = range;
		prev_length = length;
		offset = 0;
	}

	colors_use(w, COLOR_STATUS_ALERT);

	auto i = std::next(range.begin(), offset);
	for (unsigned y = 0; y < screen->main_window.size.height; ++y, ++i) {
		wmove(w, y, 0);
		if (i == range.end())
			break;

		const char *value = i->c_str();
		waddstr(w, CompletionDisplayString(value));
		wclrtoeol(w);
	}

	wclrtobot(w);

	wrefresh(w);
	colors_use(w, COLOR_LIST);
}
