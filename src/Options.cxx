/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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

#include "Options.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "config.h"
#include "charset.hxx"
#include "ConfigFile.hxx"
#include "i18n.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ERROR_UNKNOWN_OPTION    0x01
#define ERROR_BAD_ARGUMENT      0x02
#define ERROR_GOT_ARGUMENT      0x03
#define ERROR_MISSING_ARGUMENT  0x04

struct OptionDefinition {
	int shortopt;
	const char *longopt;
	const char *argument;
	const char *descrition;
};

Options options;

static constexpr OptionDefinition option_table[] = {
	{ '?', "help", nullptr, "Show this help message" },
	{ 'V', "version", nullptr, "Display version information" },
	{ 'c', "colors", nullptr, "Enable colors" },
	{ 'C', "no-colors", nullptr, "Disable colors" },
#ifdef HAVE_GETMOUSE
	{ 'm', "mouse", nullptr, "Enable mouse" },
	{ 'M', "no-mouse", nullptr, "Disable mouse" },
#endif
	{ 'e', "exit", nullptr, "Exit on connection errors" },
	{ 'p', "port", "PORT", "Connect to server on port" },
	{ 'h', "host", "HOST", "Connect to server on host" },
	{ 'P', "password","PASSWORD", "Connect with password" },
	{ 'f', "config", "FILE", "Read configuration from file" },
	{ 'k', "key-file","FILE", "Read key bindings from file" },
#ifndef NDEBUG
	{ 'K', "dump-keys", nullptr, "Dump key bindings to stdout" },
#endif
};

gcc_pure
static const OptionDefinition *
FindOption(int s) noexcept
{
	for (const auto &i : option_table)
		if (s == i.shortopt)
			return &i;

	return nullptr;
}

static const OptionDefinition *
FindOption(const char *l) noexcept
{
	for (const auto &i : option_table)
		if (strcmp(l, i.longopt) == 0)
			return &i;

	return nullptr;
}

[[noreturn]]
static void
option_error(int error, const char *option, const char *arg)
{
	switch (error) {
	case ERROR_UNKNOWN_OPTION:
		fprintf(stderr, PACKAGE ": invalid option %s\n", option);
		break;
	case ERROR_BAD_ARGUMENT:
		fprintf(stderr, PACKAGE ": bad argument: %s\n", option);
		break;
	case ERROR_GOT_ARGUMENT:
		fprintf(stderr, PACKAGE ": invalid option %s=%s\n", option, arg);
		break;
	case ERROR_MISSING_ARGUMENT:
		fprintf(stderr, PACKAGE ": missing value for %s option\n", option);
		break;
	default:
		fprintf(stderr, PACKAGE ": internal error %d\n", error);
		break;
	}

	exit(EXIT_FAILURE);
}

static void
display_help()
{
	printf("Usage: %s [OPTION]...\n", PACKAGE);

	for (const auto &i : option_table) {
		char tmp[32];

		if (i.argument)
			snprintf(tmp, sizeof(tmp), "%s=%s",
				 i.longopt,
				 i.argument);
		else
			snprintf(tmp, sizeof(tmp), "%s",
				 i.longopt);

		printf("  -%c, --%-20s %s\n",
		       i.shortopt,
		       tmp,
		       i.descrition);
	}
}

static void
handle_option(int c, const char *arg)
{
	switch (c) {
	case '?': /* --help */
		display_help();
		exit(EXIT_SUCCESS);
	case 'V': /* --version */
		puts(PACKAGE " version: " VERSION "\n"
		     "build options:"
#ifdef NCMPC_MINI
		     " mini"
#endif
#ifndef NDEBUG
		     " debug"
#endif
#ifdef ENABLE_MULTIBYTE
		     " multibyte"
#endif
#ifdef HAVE_CURSES_ENHANCED
		     " wide"
#endif
#ifdef ENABLE_LOCALE
		     " locale"
#endif
#ifdef HAVE_ICONV
		     " iconv"
#endif
#ifdef ENABLE_NLS
		     " nls"
#endif
#ifdef ENABLE_COLORS
		     " colors"
#else
		     " no-colors"
#endif
#ifdef ENABLE_LIRC
		     " lirc"
#endif
#ifdef HAVE_GETMOUSE
		     " getmouse"
#endif
#ifdef ENABLE_LIBRARY_PAGE
		     " artist-screen"
#endif
#ifdef ENABLE_HELP_SCREEN
		     " help-screen"
#endif
#ifdef ENABLE_SEARCH_SCREEN
		     " search-screen"
#endif
#ifdef ENABLE_SONG_SCREEN
		     " song-screen"
#endif
#ifdef ENABLE_KEYDEF_SCREEN
		     " key-screen"
#endif
#ifdef ENABLE_LYRICS_SCREEN
		     " lyrics-screen"
#endif
#ifdef ENABLE_OUTPUTS_SCREEN
		     " outputs-screen"
#endif
#ifdef ENABLE_CHAT_SCREEN
		     " chat-screen"
#endif

		     "\n");
#ifndef NCMPC_MINI
		printf("configuration files:\n"
		       " %s\n"
#ifndef _WIN32
		       " %s\n"
#endif
		       " %s\n\n",
		       GetUserConfigPath().c_str(),
#ifndef _WIN32
		       GetHomeConfigPath().c_str(),
#endif
		       GetSystemConfigPath().c_str());

		if (strcmp("translator-credits", _("translator-credits")) != 0)
			/* To translators: these credits are shown
			   when ncmpc is started with "--version" */
			printf("\n%s\n", _("translator-credits"));
#endif
		exit(EXIT_SUCCESS);
	case 'c': /* --colors */
#ifdef ENABLE_COLORS
		options.enable_colors = true;
#endif
		break;
	case 'C': /* --no-colors */
#ifdef ENABLE_COLORS
		options.enable_colors = false;
#endif
		break;
	case 'm': /* --mouse */
#ifdef HAVE_GETMOUSE
		options.enable_mouse = true;
#endif
		break;
	case 'M': /* --no-mouse */
#ifdef HAVE_GETMOUSE
		options.enable_mouse = false;
#endif
		break;
	case 'e': /* --exit */
		/* deprecated */
		break;
	case 'p': /* --port */
		options.port = atoi(arg);
		break;
	case 'h': /* --host */
		options.host = arg;
		break;
	case 'P': /* --password */
		options.password = LocaleToUtf8(arg).c_str();
		break;
	case 'f': /* --config */
		options.config_file = arg;
		break;
	case 'k': /* --key-file */
		options.key_file = arg;
		break;
#if !defined(NDEBUG) && !defined(NCMPC_MINI)
	case 'K': /* --dump-keys */
		read_configuration();
		GetGlobalKeyBindings().WriteToFile(stdout,
						   KEYDEF_WRITE_ALL | KEYDEF_COMMENT_ALL);
		exit(EXIT_SUCCESS);
		break;
#endif
	default:
		fprintf(stderr,"Unknown Option %c = %s\n", c, arg);
		break;
	}
}

void
options_parse(int argc, const char *argv[])
{
	const OptionDefinition *opt = nullptr;

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		size_t len = strlen(arg);

		/* check for a long option */
		if (strncmp(arg, "--", 2) == 0) {
			/* make sure we got an argument for the previous option */
			if( opt && opt->argument )
				option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);

			/* retrieve a option argument */
			std::string name;
			const char *value = strrchr(arg + 2, '=');
			if (value != nullptr) {
				name.assign(arg, value);
				value++;
			} else
				name = arg;

			/* check if the option exists */
			if ((opt = FindOption(name.c_str() + 2)) == nullptr)
				option_error(ERROR_UNKNOWN_OPTION, name.c_str(), nullptr);

			/* abort if we got an argument to the option and don't want one */
			if( value && opt->argument==nullptr )
				option_error(ERROR_GOT_ARGUMENT, arg, value);

			/* execute option callback */
			if (value || opt->argument==nullptr) {
				handle_option(opt->shortopt, value);
				opt = nullptr;
			}
		}
		/* check for short options */
		else if (len >= 2 && *arg == '-') {
			size_t j;

			for(j=1; j<len; j++) {
				/* make sure we got an argument for the previous option */
				if (opt && opt->argument)
					option_error(ERROR_MISSING_ARGUMENT,
						     opt->longopt, opt->argument);

				/* check if the option exists */
				if ((opt = FindOption(arg[j])) == nullptr)
					option_error(ERROR_UNKNOWN_OPTION, arg, nullptr);

				/* if no option argument is needed execute callback */
				if (opt->argument == nullptr) {
					handle_option(opt->shortopt, nullptr);
					opt = nullptr;
				}
			}
		} else {
			/* is this a option argument? */
			if (opt && opt->argument) {
				handle_option (opt->shortopt, arg);
				opt = nullptr;
			} else
				option_error(ERROR_BAD_ARGUMENT, arg, nullptr);
		}
	}

	if (opt && opt->argument == nullptr)
		handle_option(opt->shortopt, nullptr);
	else if (opt && opt->argument)
		option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);

	if (options.host.empty() && getenv("MPD_HOST"))
		options.host = getenv("MPD_HOST");
}
