/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2020 The Music Player Daemon Project
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

#include "ConfigParser.hxx"
#include "config.h"
#include "Bindings.hxx"
#include "KeyName.hxx"
#include "GlobalBindings.hxx"
#include "defaults.hxx"
#include "i18n.h"
#include "Command.hxx"
#include "Styles.hxx"
#include "BasicColors.hxx"
#include "CustomColors.hxx"
#include "screen_list.hxx"
#include "PageMeta.hxx"
#include "Options.hxx"
#include "util/CharUtil.hxx"
#include "util/Compiler.h"
#include "util/PrintException.hxx"
#include "util/RuntimeError.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringStrip.hxx"

#include <algorithm>
#include <array>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_LINE_LENGTH 1024
#define COMMENT_TOKEN '#'

/* configuration field names */
#define CONF_ENABLE_COLORS "enable-colors"
#define CONF_SCROLL_OFFSET "scroll-offset"
#define CONF_AUTO_CENTER "auto-center"
#define CONF_WIDE_CURSOR "wide-cursor"
#define CONF_KEY_DEFINITION "key"
#define CONF_COLOR "color"
#define CONF_COLOR_DEFINITION "colordef"
#define CONF_LIST_FORMAT "list-format"
#define CONF_SEARCH_FORMAT "search-format"
#define CONF_STATUS_FORMAT "status-format"
#define CONF_XTERM_TITLE_FORMAT "xterm-title-format"
#define CONF_LIST_WRAP "wrap-around"
#define CONF_FIND_WRAP "find-wrap"
#define CONF_FIND_SHOW_LAST "find-show-last"
#define CONF_AUDIBLE_BELL "audible-bell"
#define CONF_VISIBLE_BELL "visible-bell"
#define CONF_BELL_ON_WRAP "bell-on-wrap"
#define CONF_STATUS_MESSAGE_TIME "status-message-time"
#define CONF_XTERM_TITLE "set-xterm-title"
#define CONF_ENABLE_MOUSE "enable-mouse"
#define CONF_CROSSFADE_TIME "crossfade-time"
#define CONF_SEARCH_MODE "search-mode"
#define CONF_HIDE_CURSOR "hide-cursor"
#define CONF_SEEK_TIME "seek-time"
#define CONF_LIBRARY_PAGE_TAGS "library-page-tags"
#define CONF_SCREEN_LIST "screen-list"
#define CONF_TIMEDISPLAY_TYPE "timedisplay-type"
#define CONF_HOST "host"
#define CONF_PORT "port"
#define CONF_PASSWORD "password"
#define CONF_TIMEOUT "timeout"
#define CONF_LYRICS_TIMEOUT "lyrics-timeout"
#define CONF_SCROLL "scroll"
#define CONF_SCROLL_SEP "scroll-sep"
#define CONF_VISIBLE_BITRATE "visible-bitrate"
#define CONF_HARDWARE_CURSOR "hardware-cursor"
#define CONF_WELCOME_SCREEN_LIST "welcome-screen-list"
#define CONF_DISPLAY_TIME "display-time"
#define CONF_JUMP_PREFIX_ONLY "jump-prefix-only"
#define CONF_LYRICS_AUTOSAVE "lyrics-autosave"
#define CONF_LYRICS_SHOW_PLUGIN "lyrics-show-plugin"
#define CONF_TEXT_EDITOR "text-editor"
#define CONF_TEXT_EDITOR_ASK "text-editor-ask"
#define CONF_CHAT_PREFIX "chat-prefix"
#define CONF_SECOND_COLUMN "second-column"

gcc_pure
static bool
str2bool(char *str) noexcept
{
	return strcasecmp(str, "yes") == 0 || strcasecmp(str, "true") == 0 ||
		strcasecmp(str, "on") == 0 || strcasecmp(str, "1") == 0;
}

static constexpr bool
is_word_char(char ch) noexcept
{
	return IsAlphaNumericASCII(ch) || ch == '-' || ch == '_';
}

static char *
after_unquoted_word(char *p)
{
	if (!is_word_char(*p))
		throw FormatRuntimeError("%s: %s",
					 _("Word expected"), p);

	++p;

	while (is_word_char(*p))
		++p;

	return p;
}

/**
 * Throws on error.
 */
static int
parse_key_value(const char *str, const char **end)
{
	auto result = ParseKeyName(str);
	if (result.first == -1)
		throw FormatRuntimeError("%s: %s",
					 _("Malformed hotkey definition"),
					 result.second);

	*end = result.second;
	return result.first;
}

/**
 * Throws on error.
 */
static void
parse_key_definition(char *str)
{
	/* get the command name */
	char *eq = strchr(str, '=');
	if (eq == nullptr)
		throw FormatRuntimeError("%s: %s",
					 /* the hotkey configuration
					    line is incomplete */
					 _("Incomplete hotkey configuration"),
					 str);

	char *command_name = str;
	str = StripLeft(eq + 1);

	*eq = '\0';
	StripRight(command_name);
	const auto cmd = get_key_command_from_name(command_name);
	if (cmd == Command::NONE)
		throw FormatRuntimeError("%s: %s",
					 /* the hotkey configuration
					    contains an unknown
					    command */
					 _("Unknown command"), command_name);

	/* parse key values */
	size_t i = 0;
	const char *p = str;

	std::array<int, MAX_COMMAND_KEYS> keys{};
	while (i < MAX_COMMAND_KEYS && *p != 0) {
		keys[i++] = parse_key_value(p, &p);
		while (*p==',' || *p==' ' || *p=='\t')
			p++;
	}

	GetGlobalKeyBindings().SetKey(cmd, keys);
}

/**
 * Throws on error.
 */
static bool
parse_timedisplay_type(const char *str)
{
	if (strcmp(str, "elapsed") == 0)
		return false;
	else if (strcmp(str, "remaining") == 0)
		return true;
	else
		throw FormatRuntimeError("%s: %s",
					 /* translators: ncmpc
					    supports displaying the
					    "elapsed" or "remaining"
					    time of a song being
					    played; in this case, the
					    configuration file
					    contained an invalid
					    setting */
					 _("Bad time display type"), str);
}

#ifdef ENABLE_COLORS
/**
 * Throws on error.
 */
static char *
separate_value(char *p)
{
	char *value = strchr(p, '=');
	if (value == nullptr)
		/* an equals sign '=' was expected while parsing a
		   configuration file line */
		throw std::runtime_error(_("Missing '='"));

	*StripRight(p, value++) = '\0';

	return Strip(value);
}

/**
 * Throws on error.
 */
static void
parse_color(char *str)
{
	char *value = separate_value(str);
	ModifyStyle(str, value);
}

/**
 * Returns the first non-whitespace character after the next comma
 * character, or the end of the string.  This is used to parse comma
 * separated values.
 */
static char *
after_comma(char *p) noexcept
{
	char *comma = strchr(p, ',');

	if (comma != nullptr) {
		*comma++ = 0;
		comma = StripLeft(comma);
	} else
		comma = p + strlen(p);

	StripRight(p);
	return comma;
}

/**
 * Throws on error.
 */
static void
parse_color_definition(char *str)
{
	char *value = separate_value(str);

	/* get the command name */
	short color = ParseColorNameOrNumber(str);
	if (color < 0)
		throw FormatRuntimeError("%s: %s",
					 _("Bad color name"), str);

	/* parse r,g,b values */

	short rgb[3];
	for (unsigned i = 0; i < 3; ++i) {
		char *next = after_comma(value), *endptr;
		if (*value == 0)
			throw FormatRuntimeError("%s: %s",
						 _("Incomplete color definition"),
						 str);

		rgb[i] = strtol(value, &endptr, 0);
		if (endptr == value || *endptr != 0)
			throw FormatRuntimeError("%s: %s",
						 _("Invalid number"), value);

		value = next;
	}

	if (*value != 0)
		throw FormatRuntimeError("%s: %s",
					 _("Malformed color definition"), str);

	colors_define(color, rgb[0], rgb[1], rgb[2]);
}
#endif

static std::string
GetStringValue(const char *s) noexcept
{
	size_t length = strlen(s);

	if (s[0] == '\"' && s[length - 1] == '\"') {
		length -= 2;
		++s;
	}

	return {s, length};
}

static constexpr bool
IsListSeparator(char ch) noexcept
{
	return IsWhitespaceFast(ch) || ch == ',';
}

static char *
NextItem(char *&src) noexcept
{
	while (*src && IsListSeparator(*src))
		++src;

	if (!*src)
		return nullptr;

	char *value = src;

	while (!IsListSeparator(*src))
		++src;

	if (*src)
		*src++ = 0;

	return value;
}

/**
 * Throws on error.
 */
static std::vector<std::string>
check_screen_list(char *value)
{
	std::vector<std::string> screen;

	while (char *name = NextItem(value)) {
		std::transform(name, name + strlen(name), name, tolower);

		const auto *page_meta = screen_lookup_name(name);
		if (page_meta == nullptr)
			throw FormatRuntimeError("%s: %s",
						 /* an unknown screen
						    name was specified
						    in the
						    configuration
						    file */
						 _("Unknown screen name"),
						 name);

		/* use PageMeta::name because
		   screen_lookup_name() may have translated a
		   deprecated name */
		screen.emplace_back(page_meta->name);
	}

	if (screen.empty())
		screen = DEFAULT_SCREEN_LIST;

	return screen;
}

#ifdef ENABLE_LIBRARY_PAGE

/**
 * Throws on error.
 */
static auto
ParseTagList(char *value)
{
	std::vector<enum mpd_tag_type> result;

	while (char *name = NextItem(value)) {
		auto type = mpd_tag_name_iparse(name);
		if (type == MPD_TAG_UNKNOWN)
			throw FormatRuntimeError("%s: %s",
						 _("Unknown MPD tag"), name);

		result.emplace_back(type);
	}

	if (result.empty())
		throw std::runtime_error("Empty tag list");

	return result;
}

#endif

/**
 * Throws on error.
 */
static int
get_search_mode(char *value)
{
	char * test;
	const int mode = strtol(value, &test, 10);
	if (*test == '\0')
	{
		if (0 <= mode && mode <= 4)
			return mode;
		else
			throw FormatRuntimeError("%s: %s",
						 _("Invalid search mode"),value);
	}
	else
	{
		// TODO: modify screen_search so that its own list of modes can be used
		// for comparison instead of specifying them here
		if (strcasecmp(value, "title") == 0)
			return 0;
		else if (strcasecmp(value, "artist") == 0)
			return 1;
		else if (strcasecmp(value, "album") == 0)
			return 2;
		else if (strcasecmp(value, "filename") == 0)
			return 3;
		else if (strcasecmp(value, "artist+album") == 0)
			return 4;
		else
			throw FormatRuntimeError("%s: %s",
						 _("Unknown search mode"),
						 value);
	}
}

/**
 * Throws on error.
 */
static void
parse_line(char *line)
{
	/* get the name part */
	char *const name = line;
	char *const name_end = after_unquoted_word(line);

	line = StripLeft(name_end);
	if (*line == '=') {
		++line;
		line = StripLeft(line);
	} else if (line == name_end) {
		throw FormatRuntimeError("%s: %s",
					 _("Missing '='"), name_end);
	}

	*name_end = 0;

	/* get the value part */
	char *const value = line;

	/* key definition */
	if (!strcasecmp(CONF_KEY_DEFINITION, name))
		parse_key_definition(value);
	/* enable colors */
	else if(!strcasecmp(CONF_ENABLE_COLORS, name))
#ifdef ENABLE_COLORS
		options.enable_colors = str2bool(value);
#else
	{}
#endif
	else if (!strcasecmp(CONF_SCROLL_OFFSET, name))
		options.scroll_offset = atoi(value);
	/* auto center */
	else if (!strcasecmp(CONF_AUTO_CENTER, name))
		options.auto_center = str2bool(value);
	/* color assignment */
	else if (!strcasecmp(CONF_COLOR, name))
#ifdef ENABLE_COLORS
		parse_color(value);
#else
	{}
#endif
	/* wide cursor */
	else if (!strcasecmp(CONF_WIDE_CURSOR, name))
		options.wide_cursor = str2bool(value);
	else if (strcasecmp(name, CONF_HARDWARE_CURSOR) == 0)
		options.hardware_cursor = str2bool(value);
	/* welcome screen list */
	else if (!strcasecmp(CONF_WELCOME_SCREEN_LIST, name))
		options.welcome_screen_list = str2bool(value);
	/* visible bitrate */
	else if (!strcasecmp(CONF_VISIBLE_BITRATE, name))
		options.visible_bitrate = str2bool(value);
	/* timer display type */
	else if (!strcasecmp(CONF_TIMEDISPLAY_TYPE, name))
		options.display_remaining_time = parse_timedisplay_type(value);
		/* color definition */
	else if (!strcasecmp(CONF_COLOR_DEFINITION, name))
#ifdef ENABLE_COLORS
		parse_color_definition(value);
#else
	{}
#endif
	/* list format string */
	else if (!strcasecmp(CONF_LIST_FORMAT, name)) {
		options.list_format = GetStringValue(value);
		/* search format string */
	} else if (!strcasecmp(CONF_SEARCH_FORMAT, name)) {
		options.search_format = GetStringValue(value);
		/* status format string */
	} else if (!strcasecmp(CONF_STATUS_FORMAT, name)) {
		options.status_format = GetStringValue(value);
		/* xterm title format string */
	} else if (!strcasecmp(CONF_XTERM_TITLE_FORMAT, name)) {
		options.xterm_title_format = GetStringValue(value);
	} else if (!strcasecmp(CONF_LIST_WRAP, name))
		options.list_wrap = str2bool(value);
	else if (!strcasecmp(CONF_FIND_WRAP, name))
		options.find_wrap = str2bool(value);
	else if (!strcasecmp(CONF_FIND_SHOW_LAST,name))
		options.find_show_last_pattern = str2bool(value);
	else if (!strcasecmp(CONF_AUDIBLE_BELL, name))
		options.audible_bell = str2bool(value);
	else if (!strcasecmp(CONF_VISIBLE_BELL, name))
		options.visible_bell = str2bool(value);
	else if (!strcasecmp(CONF_BELL_ON_WRAP, name))
		options.bell_on_wrap = str2bool(value);
	else if (!strcasecmp(CONF_STATUS_MESSAGE_TIME, name))
		options.status_message_time = std::chrono::seconds(atoi(value));
	else if (!strcasecmp(CONF_XTERM_TITLE, name))
		options.enable_xterm_title = str2bool(value);
	else if (!strcasecmp(CONF_ENABLE_MOUSE, name))
#ifdef HAVE_GETMOUSE
		options.enable_mouse = str2bool(value);
#else
	{}
#endif
	else if (!strcasecmp(CONF_CROSSFADE_TIME, name))
		options.crossfade_time = atoi(value);
	else if (!strcasecmp(CONF_SEARCH_MODE, name))
		options.search_mode = get_search_mode(value);
	else if (!strcasecmp(CONF_HIDE_CURSOR, name))
		options.hide_cursor = std::chrono::seconds(atoi(value));
	else if (!strcasecmp(CONF_SEEK_TIME, name))
		options.seek_time = atoi(value);
	else if (!strcasecmp(CONF_LIBRARY_PAGE_TAGS, name)) {
#ifdef ENABLE_LIBRARY_PAGE
		options.library_page_tags = ParseTagList(value);
#endif
	} else if (!strcasecmp(CONF_SCREEN_LIST, name)) {
		options.screen_list = check_screen_list(value);
	} else if (!strcasecmp(CONF_HOST, name))
		options.host = GetStringValue(value);
	else if (!strcasecmp(CONF_PORT, name))
		options.port = atoi(GetStringValue(value).c_str());
	else if (!strcasecmp(CONF_PASSWORD, name))
		options.password = GetStringValue(value);
	else if (!strcasecmp(CONF_TIMEOUT, name))
		options.timeout_ms = atoi(GetStringValue(value).c_str())
				     * 1000 /* seconds -> milliseconds */;
	else if (!strcasecmp(CONF_LYRICS_TIMEOUT, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_timeout = std::chrono::seconds(atoi(GetStringValue(value).c_str()));
#else
	{}
#endif
	else if (!strcasecmp(CONF_SCROLL, name))
		options.scroll = str2bool(value);
	else if (!strcasecmp(CONF_SCROLL_SEP, name)) {
		options.scroll_sep = GetStringValue(value);
	} else if (!strcasecmp(CONF_DISPLAY_TIME, name))
		/* obsolete, ignore */
		{}
	else if (!strcasecmp(CONF_JUMP_PREFIX_ONLY, name))
#ifdef NCMPC_MINI
		{}
#else
		options.jump_prefix_only = str2bool(value);
#endif
	else if (!strcasecmp(CONF_LYRICS_AUTOSAVE, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_autosave = str2bool(value);
#else
	{}
#endif
	else if (!strcasecmp(CONF_LYRICS_SHOW_PLUGIN, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_show_plugin = str2bool(value);
#else
		{}
#endif
	else if (!strcasecmp(name, CONF_TEXT_EDITOR))
#ifdef ENABLE_LYRICS_SCREEN
		{
			options.text_editor = GetStringValue(value);
		}
#else
		{}
#endif
	else if (!strcasecmp(name, CONF_TEXT_EDITOR_ASK))
#ifdef ENABLE_LYRICS_SCREEN
		options.text_editor_ask = str2bool(value);
#else
		{}
#endif
	else if (!strcasecmp(name, CONF_CHAT_PREFIX))
#ifdef ENABLE_CHAT_SCREEN
		options.chat_prefix = GetStringValue(value);
#else
		{}
#endif
	else if (!strcasecmp(CONF_SECOND_COLUMN, name))
#ifdef NCMPC_MINI
		{}
#else
		options.second_column = str2bool(value);
#endif
	else
		throw FormatRuntimeError("%s: %s",
					 _("Unknown configuration parameter"),
					 name);
}

bool
ReadConfigFile(const char *filename)
{
	assert(filename != nullptr);

	FILE *file = fopen(filename, "r");
	if (file == nullptr) {
		perror(filename);
		return false;
	}

	AtScopeExit(file) { fclose(file); };

	unsigned no = 0;
	char line[MAX_LINE_LENGTH];
	while (fgets(line, sizeof(line), file) != nullptr) {
		++no;
		char *p = StripLeft(line);
		if (*p == 0 || *p == COMMENT_TOKEN)
			continue;

		StripRight(p);

		try {
			parse_line(p);
		} catch (...) {
			fprintf(stderr,
				"Failed to parse '%s' line %u: ",
				filename, no);
			PrintException(std::current_exception());
		}
	}

	return true;
}
