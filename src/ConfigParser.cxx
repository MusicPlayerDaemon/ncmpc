// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

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
#include "ui/Options.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "util/CharUtil.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringAPI.hxx"
#include "util/StringStrip.hxx"

#ifndef NCMPC_MINI
#include "TableGlue.hxx"
#include "TableStructure.hxx"
#endif

#include <algorithm>
#include <array>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

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

[[gnu::pure]]
static bool
str2bool(char *str) noexcept
{
	return StringIsEqualIgnoreCase(str, "yes") ||
		StringIsEqualIgnoreCase(str, "true") ||
		StringIsEqualIgnoreCase(str, "on") ||
		StringIsEqualIgnoreCase(str, "1");
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
		throw FmtRuntimeError("{}: {:?}",
				      _("Word expected"), p);

	++p;

	while (is_word_char(*p))
		++p;

	return p;
}

#ifndef NCMPC_MINI

static constexpr bool
IsValueChar(char ch) noexcept
{
	return IsAlphaNumericASCII(ch) || ch == '-' || ch == '_' || ch == '.';
}

[[gnu::pure]]
static char *
AfterUnquotedValue(char *p) noexcept
{
	while (IsValueChar(*p))
		++p;

	return p;
}

/**
 * Throws on error.
 */
static char *
NextUnquotedValue(char *&pp)
{
	char *value = pp;

	char *end = AfterUnquotedValue(value);
	if (*end == 0) {
		pp = end;
	} else if (IsWhitespaceFast(*end)) {
		*end = 0;
		pp = StripLeft(end + 1);
	} else
		throw FmtRuntimeError("{}: {:?}",
				      _("Whitespace expected"), end);

	return value;
}

/**
 * Throws on error.
 */
static char *
NextQuotedValue(char *&pp)
{
	char *p = pp;
	if (*p != '"')
		throw FmtRuntimeError("{}: {:?}",
				      _("Quoted value expected"), p);

	++p;

	char *const result = p;

	char *end = strchr(p, '"');
	if (end == nullptr)
		throw FmtRuntimeError("{}: {:?}",
				      _("Closing quote missing"), p);

	*end = 0;
	pp = end + 1;
	return result;
}

/**
 * Throws on error.
 */
static std::pair<char *, char *>
NextNameValue(char *&p)
{
	char *name = p;

	p = after_unquoted_word(p);
	if (*p != '=')
		throw FmtRuntimeError("{}: {:?}",
				      _("Syntax error"), p);

	*p++ = 0;

	char *value = NextUnquotedValue(p);

	return std::make_pair(name, value);
}

#endif

/**
 * Throws on error.
 */
static int
parse_key_value(const char *str, const char **end)
{
	auto result = ParseKeyName(str);
	if (result.first == -1)
		throw FmtRuntimeError("{}: {:?}",
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
		throw FmtRuntimeError("{}: {:?}",
				      /* the hotkey configuration line
					 is incomplete */
				      _("Incomplete hotkey configuration"),
				      str);

	char *command_name = str;
	str = StripLeft(eq + 1);

	*eq = '\0';
	StripRight(command_name);
	const auto cmd = get_key_command_from_name(command_name);
	if (cmd == Command::NONE)
		throw FmtRuntimeError("{}: {:?}",
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
static CurrentTimeDisplay
ParseCurrentTimeDisplay(const char *str)
{
	if (StringIsEqual(str, "elapsed"))
		return CurrentTimeDisplay::ELAPSED;
	else if (StringIsEqual(str, "remaining"))
		return CurrentTimeDisplay::REMAINING;
	else if (StringIsEqual(str, "none"))
		return CurrentTimeDisplay::NONE;
	else
		throw FmtRuntimeError("{}: {:?}",
				      /* translators: ncmpc supports
					 displaying the "elapsed" or
					 "remaining" time of a song
					 being played; in this case,
					 the configuration file
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
		throw FmtRuntimeError("{}: {:?}",
				      _("Bad color name"), str);

	/* parse r,g,b values */

	short rgb[3];
	for (unsigned i = 0; i < 3; ++i) {
		char *next = after_comma(value), *endptr;
		if (*value == 0)
			throw FmtRuntimeError("{}: {:?}",
					      _("Incomplete color definition"),
					      str);

		rgb[i] = strtol(value, &endptr, 0);
		if (endptr == value || *endptr != 0)
			throw FmtRuntimeError("{}: {:?}",
					      _("Invalid number"), value);

		value = next;
	}

	if (*value != 0)
		throw FmtRuntimeError("{}: {:?}",
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
			throw FmtRuntimeError("{}: {:?}",
					      /* an unknown screen
						 name was specified in
						 the configuration
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
			throw FmtRuntimeError("{}: {:?}",
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
			throw FmtRuntimeError("{}: {:?}",
					      _("Invalid search mode"),value);
	}
	else
	{
		// TODO: modify screen_search so that its own list of modes can be used
		// for comparison instead of specifying them here
		if (StringIsEqualIgnoreCase(value, "title"))
			return 0;
		else if (StringIsEqualIgnoreCase(value, "artist"))
			return 1;
		else if (StringIsEqualIgnoreCase(value, "album"))
			return 2;
		else if (StringIsEqualIgnoreCase(value, "filename"))
			return 3;
		else if (StringIsEqualIgnoreCase(value, "artist+album"))
			return 4;
		else
			throw FmtRuntimeError("{}: {:?}",
					      _("Unknown search mode"),
					      value);
	}
}

#ifndef NCMPC_MINI

/**
 * Throws on error.
 */
static TableColumn
ParseTableColumn(char *s)
{
	TableColumn column;

	column.caption = NextQuotedValue(s);
	s = StripLeft(s);
	column.format = NextQuotedValue(s);
	s = StripLeft(s);

	while (*s != 0) {
		auto nv = NextNameValue(s);
		s = StripLeft(s);

		const char *name = nv.first;
		const char *value = nv.second;

		if (StringIsEqual(name, "min")) {
			char *endptr;
			column.min_width = strtoul(value, &endptr, 10);
			if (endptr == value || *endptr != 0 ||
			    column.min_width == 0 || column.min_width > 1000)
				throw FmtRuntimeError("{}: {:?}",
						      _("Invalid column width"),
						      value);
		} else if (StringIsEqual(name, "fraction")) {
			char *endptr;
			column.fraction_width = strtod(value, &endptr);
			if (endptr == value || *endptr != 0 ||
			    column.fraction_width < 0 ||
			    column.fraction_width > 1000)
				throw FmtRuntimeError("{}: {:?}",
						      _("Invalid column fraction width"),
						      value);
		}
	}

	return column;
}

/**
 * Throws on error.
 */
static void
ParseTableColumn(TableStructure &t, char *s)
{
	t.columns.emplace_back(ParseTableColumn(s));
}

#endif

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
		throw FmtRuntimeError("{}: {:?}",
				      _("Missing '='"), name_end);
	}

	*name_end = 0;

	/* get the value part */
	char *const value = line;

	/* key definition */
	if (StringIsEqualIgnoreCase(CONF_KEY_DEFINITION, name))
		parse_key_definition(value);
	else if (StringIsEqualIgnoreCase("show-title-bar", name))
		options.show_title_bar = str2bool(value);
	/* enable colors */
	else if(StringIsEqualIgnoreCase(CONF_ENABLE_COLORS, name))
#ifdef ENABLE_COLORS
		ui_options.enable_colors = str2bool(value);
#else
	{}
#endif
	else if (StringIsEqualIgnoreCase(CONF_SCROLL_OFFSET, name))
		ui_options.scroll_offset = atoi(value);
	/* auto center */
	else if (StringIsEqualIgnoreCase(CONF_AUTO_CENTER, name))
		options.auto_center = str2bool(value);
	/* color assignment */
	else if (StringIsEqualIgnoreCase(CONF_COLOR, name))
#ifdef ENABLE_COLORS
		parse_color(value);
#else
	{}
#endif
	/* wide cursor */
	else if (StringIsEqualIgnoreCase(CONF_WIDE_CURSOR, name))
		ui_options.wide_cursor = str2bool(value);
	else if (StringIsEqualIgnoreCase(name, CONF_HARDWARE_CURSOR))
		ui_options.hardware_cursor = str2bool(value);
	/* welcome screen list */
	else if (StringIsEqualIgnoreCase(CONF_WELCOME_SCREEN_LIST, name))
		options.welcome_screen_list = str2bool(value);
	/* visible bitrate */
	else if (StringIsEqualIgnoreCase(CONF_VISIBLE_BITRATE, name))
		options.visible_bitrate = str2bool(value);
	/* timer display type */
	else if (StringIsEqualIgnoreCase(CONF_TIMEDISPLAY_TYPE, name))
		options.current_time_display = ParseCurrentTimeDisplay(value);
		/* color definition */
	else if (StringIsEqualIgnoreCase(CONF_COLOR_DEFINITION, name))
#ifdef ENABLE_COLORS
		parse_color_definition(value);
#else
	{}
#endif
	/* list format string */
	else if (StringIsEqualIgnoreCase(CONF_LIST_FORMAT, name)) {
		options.list_format = GetStringValue(value);
	} else if (StringIsEqualIgnoreCase("song-table-column", name)) {
#ifndef NCMPC_MINI
		ParseTableColumn(song_table_structure, value);
#endif

		/* search format string */
	} else if (StringIsEqualIgnoreCase(CONF_SEARCH_FORMAT, name)) {
		options.search_format = GetStringValue(value);
		/* status format string */
	} else if (StringIsEqualIgnoreCase(CONF_STATUS_FORMAT, name)) {
		options.status_format = GetStringValue(value);
		/* xterm title format string */
	} else if (StringIsEqualIgnoreCase(CONF_XTERM_TITLE_FORMAT, name)) {
		options.xterm_title_format = GetStringValue(value);
	} else if (StringIsEqualIgnoreCase(CONF_LIST_WRAP, name))
		ui_options.list_wrap = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_FIND_WRAP, name))
		ui_options.find_wrap = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_FIND_SHOW_LAST,name))
		ui_options.find_show_last_pattern = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_AUDIBLE_BELL, name))
		ui_options.audible_bell = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_VISIBLE_BELL, name))
		ui_options.visible_bell = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_BELL_ON_WRAP, name))
		ui_options.bell_on_wrap = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_STATUS_MESSAGE_TIME, name))
		options.status_message_time = std::chrono::seconds(atoi(value));
	else if (StringIsEqualIgnoreCase(CONF_XTERM_TITLE, name))
		options.enable_xterm_title = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_ENABLE_MOUSE, name))
#ifdef HAVE_GETMOUSE
		options.enable_mouse = str2bool(value);
#else
	{}
#endif
	else if (StringIsEqualIgnoreCase(CONF_CROSSFADE_TIME, name))
		options.crossfade_time = atoi(value);
	else if (StringIsEqualIgnoreCase(CONF_SEARCH_MODE, name))
		options.search_mode = get_search_mode(value);
	else if (StringIsEqualIgnoreCase(CONF_HIDE_CURSOR, name))
		options.hide_cursor = std::chrono::seconds(atoi(value));
	else if (StringIsEqualIgnoreCase(CONF_SEEK_TIME, name))
		options.seek_time = atoi(value);
	else if (StringIsEqualIgnoreCase(CONF_LIBRARY_PAGE_TAGS, name)) {
#ifdef ENABLE_LIBRARY_PAGE
		options.library_page_tags = ParseTagList(value);
#endif
	} else if (StringIsEqualIgnoreCase(CONF_SCREEN_LIST, name)) {
		options.screen_list = check_screen_list(value);
	} else if (StringIsEqualIgnoreCase(CONF_HOST, name))
		options.host = GetStringValue(value);
	else if (StringIsEqualIgnoreCase(CONF_PORT, name))
		options.port = atoi(GetStringValue(value).c_str());
	else if (StringIsEqualIgnoreCase(CONF_PASSWORD, name))
		options.password = GetStringValue(value);
	else if (StringIsEqualIgnoreCase(CONF_TIMEOUT, name))
		options.timeout_ms = atoi(GetStringValue(value).c_str())
				     * 1000 /* seconds -> milliseconds */;
	else if (StringIsEqualIgnoreCase(CONF_LYRICS_TIMEOUT, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_timeout = std::chrono::seconds(atoi(GetStringValue(value).c_str()));
#else
	{}
#endif
	else if (StringIsEqualIgnoreCase(CONF_SCROLL, name))
		options.scroll = str2bool(value);
	else if (StringIsEqualIgnoreCase(CONF_SCROLL_SEP, name)) {
		options.scroll_sep = GetStringValue(value);
	} else if (StringIsEqualIgnoreCase(CONF_DISPLAY_TIME, name))
		/* obsolete, ignore */
		{}
	else if (StringIsEqualIgnoreCase(CONF_JUMP_PREFIX_ONLY, name))
#ifdef NCMPC_MINI
		{}
#else
		ui_options.jump_prefix_only = str2bool(value);
#endif
	else if (StringIsEqualIgnoreCase(CONF_LYRICS_AUTOSAVE, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_autosave = str2bool(value);
#else
	{}
#endif
	else if (StringIsEqualIgnoreCase(CONF_LYRICS_SHOW_PLUGIN, name))
#ifdef ENABLE_LYRICS_SCREEN
		options.lyrics_show_plugin = str2bool(value);
#else
		{}
#endif
	else if (StringIsEqualIgnoreCase(name, CONF_TEXT_EDITOR))
#ifdef ENABLE_LYRICS_SCREEN
		{
			options.text_editor = GetStringValue(value);
		}
#else
		{}
#endif
	else if (StringIsEqualIgnoreCase(name, CONF_TEXT_EDITOR_ASK))
		/* obsolete, ignore */
		{}
	else if (StringIsEqualIgnoreCase(name, CONF_CHAT_PREFIX))
#ifdef ENABLE_CHAT_SCREEN
		options.chat_prefix = GetStringValue(value);
#else
		{}
#endif
	else if (StringIsEqualIgnoreCase(CONF_SECOND_COLUMN, name))
#ifdef NCMPC_MINI
		{}
#else
		options.second_column = str2bool(value);
#endif
	else
		throw FmtRuntimeError("{}: {:?}",
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
			fmt::print(stderr,
				   "Failed to parse {:?} line {}: ",
				   filename, no);
			PrintException(std::current_exception());
		}
	}

	return true;
}
