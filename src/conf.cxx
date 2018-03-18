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

#include "conf.hxx"
#include "config.h"
#include "defaults.hxx"
#include "i18n.h"
#include "command.hxx"
#include "colors.hxx"
#include "screen_list.hxx"
#include "options.hxx"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

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

gcc_const
static bool
is_space_not_null(char ch)
{
	unsigned char uch = (unsigned char)ch;
	return uch <= 0x20 && uch > 0;
}

/**
 * Returns the first non-space character (or a pointer to the null
 * terminator).  Similar to g_strchug(), but just moves the pointer
 * forward without modifying the string contents.
 */
gcc_pure
static char *
skip_spaces(char *p)
{
	while (is_space_not_null(*p))
		++p;
	return p;
}

static bool
str2bool(char *str)
{
	return strcasecmp(str, "yes") == 0 || strcasecmp(str, "true") == 0 ||
		strcasecmp(str, "on") == 0 || strcasecmp(str, "1") == 0;
}

static void
print_error(const char *msg, const char *input)
{
	fprintf(stderr, "%s: %s ('%s')\n",
		/* To translators: prefix for error messages */
		_("Error"), msg, input);
}

gcc_const
static bool
is_word_char(char ch)
{
	return g_ascii_isalnum(ch) || ch == '-' || ch == '_';
}

static char *
after_unquoted_word(char *p)
{
	if (!is_word_char(*p)) {
		print_error(_("Word expected"), p);
		return nullptr;
	}

	++p;

	while (is_word_char(*p))
		++p;

	return p;
}

static int
parse_key_value(char *str, char **end)
{
	if (*str == '\'') {
		if (str[1] == '\'' || str[2] != '\'') {
			print_error(_("Malformed hotkey definition"), str);
			return -1;
		}

		*end = str + 3;
		return str[1];
	} else {
		long value = strtol(str, end, 0);
		if (*end == str) {
			print_error(_("Malformed hotkey definition"), str);
			return -1;
		}

		return (int)value;
	}
}

static bool
parse_key_definition(char *str)
{
	/* get the command name */
	const size_t len = strlen(str);
	size_t i = 0;
	int j = 0;
	char buf[MAX_LINE_LENGTH];
	memset(buf, 0, MAX_LINE_LENGTH);
	while (i < len && str[i] != '=' && !g_ascii_isspace(str[i]))
		buf[j++] = str[i++];

	command_t cmd = get_key_command_from_name(buf);
	if(cmd == CMD_NONE) {
		/* the hotkey configuration contains an unknown
		   command */
		print_error(_("Unknown command"), buf);
		return false;
	}

	/* skip whitespace */
	while (i < len && (str[i] == '=' || g_ascii_isspace(str[i])))
		i++;

	/* get the value part */
	memset(buf, 0, MAX_LINE_LENGTH);
	g_strlcpy(buf, str+i, MAX_LINE_LENGTH);
	if (*buf == 0) {
		/* the hotkey configuration line is incomplete */
		print_error(_("Incomplete hotkey configuration"), str);
		return false;
	}

	/* parse key values */
	i = 0;
	int key = 0;
	char *p = buf;

	int keys[MAX_COMMAND_KEYS];
	memset(keys, 0, sizeof(int)*MAX_COMMAND_KEYS);
	while (i < MAX_COMMAND_KEYS && *p != 0 &&
	       (key = parse_key_value(p, &p)) >= 0) {
		keys[i++] = key;
		while (*p==',' || *p==' ' || *p=='\t')
			p++;
	}

	if (key < 0)
		return false;

	return assign_keys(cmd, keys);
}

static bool
parse_timedisplay_type(const char *str)
{
	if (strcmp(str, "elapsed") == 0)
		return false;
	else if (strcmp(str, "remaining") == 0)
		return true;
	else {
		/* translators: ncmpc supports displaying the
		   "elapsed" or "remaining" time of a song being
		   played; in this case, the configuration file
		   contained an invalid setting */
		print_error(_("Bad time display type"), str);
		return false;
	}
}

#ifdef ENABLE_COLORS
static char *
separate_value(char *p)
{
	char *value = strchr(p, '=');
	if (value == nullptr) {
		/* an equals sign '=' was expected while parsing a
		   configuration file line */
		fprintf(stderr, "%s\n", _("Missing '='"));
		return nullptr;
	}

	*value++ = 0;

	g_strchomp(p);
	return skip_spaces(value);
}

static bool
parse_color(char *str)
{
	char *value = separate_value(str);
	if (value == nullptr)
		return false;

	return colors_assign(str, value);
}

/**
 * Returns the first non-whitespace character after the next comma
 * character, or the end of the string.  This is used to parse comma
 * separated values.
 */
static char *
after_comma(char *p)
{
	char *comma = strchr(p, ',');

	if (comma != nullptr) {
		*comma++ = 0;
		comma = skip_spaces(comma);
	} else
		comma = p + strlen(p);

	g_strchomp(p);
	return comma;
}

static bool
parse_color_definition(char *str)
{
	char *value = separate_value(str);
	if (value == nullptr)
		return false;

	/* get the command name */
	short color = colors_str2color(str);
	if (color < 0) {
		char buf[MAX_LINE_LENGTH];
		print_error(_("Bad color name"), buf);
		return false;
	}

	/* parse r,g,b values */

	short rgb[3];
	for (unsigned i = 0; i < 3; ++i) {
		char *next = after_comma(value), *endptr;
		if (*value == 0) {
			print_error(_("Incomplete color definition"), str);
			return false;
		}

		rgb[i] = strtol(value, &endptr, 0);
		if (endptr == value || *endptr != 0) {
			print_error(_("Invalid number"), value);
			return false;
		}

		value = next;
	}

	if (*value != 0) {
		print_error(_("Malformed color definition"), str);
		return false;
	}

	return colors_define(str, rgb[0], rgb[1], rgb[2]);
}
#endif

static std::string
GetStringValue(const char *s)
{
	size_t length = strlen(s);

	if (s[0] == '\"' && s[length - 1] == '\"') {
		length -= 2;
		++s;
	}

	return {s, length};
}

static std::vector<std::string>
check_screen_list(char *value)
{
	char **tmp = g_strsplit_set(value, " \t,", 100);
	std::vector<std::string> screen;
	int i = 0;

	while( tmp && tmp[i] ) {
		char *name = g_ascii_strdown(tmp[i], -1);
		if (*name != '\0') {
			if (screen_lookup_name(name) == nullptr) {
				/* an unknown screen name was specified in the
				   configuration file */
				print_error(_("Unknown screen name"), name);
			} else {
				screen.emplace_back(name);
			}
		}
		g_free(name);
		i++;
	}
	g_strfreev(tmp);
	if (screen.empty())
		return DEFAULT_SCREEN_LIST;

	return screen;
}

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
		{
			print_error(_("Invalid search mode"),value);
			return 0;
		}
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
		{
			print_error(_("Unknown search mode"),value);
			return 0;
		}
	}
}

static bool
parse_line(char *line)
{
	/* get the name part */
	char *const name = line;
	char *const name_end = after_unquoted_word(line);
	if (name_end == nullptr)
		return false;

	line = skip_spaces(name_end);
	if (*line == '=') {
		++line;
		line = skip_spaces(line);
	} else if (line == name_end) {
		print_error(_("Missing '='"), name_end);
		return false;
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
		options.hide_cursor = atoi(value);
	else if (!strcasecmp(CONF_SEEK_TIME, name))
		options.seek_time = atoi(value);
	else if (!strcasecmp(CONF_SCREEN_LIST, name)) {
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
		options.lyrics_timeout = atoi(GetStringValue(value).c_str());
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
	else {
		print_error(_("Unknown configuration parameter"), name);
		return false;
	}

	return true;
}

static int
read_rc_file(char *filename)
{
	assert(filename != nullptr);

	FILE *file = fopen(filename, "r");
	if (file == nullptr) {
		perror(filename);
		return -1;
	}

	char line[MAX_LINE_LENGTH];
	while (fgets(line, sizeof(line), file) != nullptr) {
		char *p = skip_spaces(line);

		if (*p != 0 && *p != COMMENT_TOKEN)
			parse_line(g_strchomp(p));
	}

	fclose(file);
	return 0;
}

bool
check_user_conf_dir()
{
	char *directory = g_build_filename(g_get_home_dir(), "." PACKAGE, nullptr);

	if (g_file_test(directory, G_FILE_TEST_IS_DIR)) {
		g_free(directory);
		return true;
	}

	bool success = g_mkdir(directory, 0755) == 0;
	g_free(directory);
	return success;
}

char *
build_user_conf_filename()
{
#ifdef WIN32
	return g_build_filename(g_get_user_config_dir(), PACKAGE, "ncmpc.conf", nullptr);
#else
	return g_build_filename(g_get_home_dir(), "." PACKAGE, "config", nullptr);
#endif
}

char *
build_system_conf_filename()
{
#ifdef WIN32
	const gchar* const *system_data_dirs;
	gchar *pathname = nullptr;

	for (system_data_dirs = g_get_system_config_dirs (); *system_data_dirs != nullptr; system_data_dirs++)
	{
		pathname = g_build_filename(*system_data_dirs, PACKAGE, "ncmpc.conf", nullptr);
		if (g_file_test(pathname, G_FILE_TEST_EXISTS))
		{
			break;
		}
		else
		{
			g_free (pathname);
			pathname = nullptr;
		}
	}
	return pathname;
#else
	return g_build_filename(SYSCONFDIR, PACKAGE, "config", nullptr);
#endif
}

char *
build_user_key_binding_filename()
{
#ifdef WIN32
	return g_build_filename(g_get_user_config_dir(), PACKAGE, "keys.conf", nullptr);
#else
	return g_build_filename(g_get_home_dir(), "." PACKAGE, "keys", nullptr);
#endif
}

static char *
g_build_system_key_binding_filename()
{
#ifdef WIN32
	const gchar* const *system_data_dirs;
	gchar *pathname = nullptr;

	for (system_data_dirs = g_get_system_config_dirs (); *system_data_dirs != nullptr; system_data_dirs++)
	{
		pathname = g_build_filename(*system_data_dirs, PACKAGE, "keys.conf", nullptr);
		if (g_file_test(pathname, G_FILE_TEST_EXISTS))
		{
			break;
		}
		else
		{
			g_free (pathname);
			pathname = nullptr;
		}
	}
	return pathname;
#else
	return g_build_filename(SYSCONFDIR, PACKAGE, "keys", nullptr);
#endif
}

static char *
find_config_file()
{
	/* check for command line configuration file */
	if (!options.config_file.empty())
		return g_strdup(options.config_file.c_str());

	/* check for user configuration ~/.ncmpc/config */
	char *filename = build_user_conf_filename();
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		return filename;

	g_free(filename);

	/* check for  global configuration SYSCONFDIR/ncmpc/config */
	filename = build_system_conf_filename();
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		return filename;

	g_free(filename);
	return nullptr;
}

static char *
find_keys_file()
{
	/* check for command line key binding file */
	if (!options.key_file.empty())
		return g_strdup(options.key_file.c_str());

	/* check for  user key bindings ~/.ncmpc/keys */
	char *filename = build_user_key_binding_filename();
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		return filename;

	g_free(filename);

	/* check for  global key bindings SYSCONFDIR/ncmpc/keys */
	filename = g_build_system_key_binding_filename();
	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
		return filename;

	g_free(filename);
	return nullptr;
}

void
read_configuration()
{
	/* load configuration */
	char *filename = find_config_file();
	if (filename != nullptr) {
		read_rc_file(filename);
		g_free(filename);
	}

	/* load key bindings */
	filename = find_keys_file();
	if (filename != nullptr) {
		read_rc_file(filename);
		g_free(filename);
	}
}
