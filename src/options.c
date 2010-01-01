/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
 * Project homepage: http://musicpd.org

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "options.h"
#include "config.h"
#include "defaults.h"
#include "charset.h"
#include "command.h"
#include "conf.h"
#include "i18n.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>

#define MAX_LONGOPT_LENGTH 32

#define ERROR_UNKNOWN_OPTION    0x01
#define ERROR_BAD_ARGUMENT      0x02
#define ERROR_GOT_ARGUMENT      0x03
#define ERROR_MISSING_ARGUMENT  0x04

typedef struct {
	int shortopt;
	const char *longopt;
	const char *argument;
	const char *descrition;
} arg_opt_t;


typedef void (*option_callback_fn_t)(int c, const char *arg);


options_t options = {
	.crossfade_time = DEFAULT_CROSSFADE_TIME,
	.seek_time = 1,
#ifdef ENABLE_LYRICS_SCREEN
	.lyrics_timeout = DEFAULT_LYRICS_TIMEOUT,
	.lyrics_autosave = false,
#endif
	.find_wrap = true,
	.scroll_offset = 0,
	.wide_cursor = true,
	.audible_bell = true,
	.bell_on_wrap = true,
	.status_message_time = 3,
#ifndef NCMPC_MINI
	.scroll = DEFAULT_SCROLL,
	.welcome_screen_list = true,
	.jump_prefix_only = true,
	.second_column = true,
#endif
};

static const arg_opt_t option_table[] = {
	{ '?', "help", NULL, "Show this help message" },
	{ 'V', "version", NULL, "Display version information" },
	{ 'c', "colors", NULL, "Enable colors" },
	{ 'C', "no-colors", NULL, "Disable colors" },
#ifdef HAVE_GETMOUSE
	{ 'm', "mouse", NULL, "Enable mouse" },
	{ 'M', "no-mouse", NULL, "Disable mouse" },
#endif
	{ 'e', "exit", NULL, "Exit on connection errors" },
	{ 'p', "port", "PORT", "Connect to server on port" },
	{ 'h', "host", "HOST", "Connect to server on host" },
	{ 'P', "password","PASSWORD", "Connect with password" },
	{ 'f', "config", "FILE", "Read configuration from file" },
	{ 'k', "key-file","FILE", "Read configuration from file" },
	{ 'S', "no-splash", NULL, "Don't show the splash screen" },
#ifndef NDEBUG
	{ 'K', "dump-keys", NULL, "Dump key bindings to stdout" },
#endif
};

static const unsigned option_table_size = sizeof(option_table) / sizeof(option_table[0]);

static const arg_opt_t *
lookup_option(int s, char *l)
{
	unsigned i;

	for (i = 0; i < option_table_size; ++i) {
		if (l && strcmp(l, option_table[i].longopt) == 0)
			return &option_table[i];;
		if (s && s == option_table[i].shortopt)
			return &option_table[i];;
	}

	return NULL;
}

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
display_help(void)
{
	unsigned i;

	printf("Usage: %s [OPTION]...\n", PACKAGE);

	for (i = 0; i < option_table_size; ++i) {
		char tmp[MAX_LONGOPT_LENGTH];

		if (option_table[i].argument)
			g_snprintf(tmp, MAX_LONGOPT_LENGTH, "%s=%s",
				   option_table[i].longopt,
				   option_table[i].argument);
		else
			g_strlcpy(tmp, option_table[i].longopt, 64);

		printf("  -%c, --%-20s %s\n",
		       option_table[i].shortopt,
		       tmp,
		       option_table[i].descrition);
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
#ifdef ENABLE_WIDE
		     " wide"
#endif
#ifdef ENABLE_LOCALE
		     " locale"
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
#ifdef ENABLE_ARTIST_SCREEN
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

		     "\n");
#ifndef NCMPC_MINI
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
		g_free(options.host);
		options.host = g_strdup(arg);
		break;
	case 'P': /* --password */
		g_free(options.password);
		options.password = locale_to_utf8(arg);
		break;
	case 'f': /* --config */
		g_free(options.config_file);
		options.config_file = g_strdup(arg);
		break;
	case 'k': /* --key-file */
		g_free(options.key_file);
		options.key_file = g_strdup(arg);
		break;
	case 'S': /* --key-file */
		/* the splash screen was removed */
		break;
#ifndef NDEBUG
#ifndef NCMPC_MINI
	case 'K': /* --dump-keys */
		read_configuration();
		write_key_bindings(stdout, KEYDEF_WRITE_ALL | KEYDEF_COMMENT_ALL);
		exit(EXIT_SUCCESS);
		break;
#endif
#endif
	default:
		fprintf(stderr,"Unknown Option %c = %s\n", c, arg);
		break;
	}
}

void
options_parse(int argc, const char *argv[])
{
	int i;
	const arg_opt_t *opt = NULL;
	option_callback_fn_t option_cb = handle_option;

	i=1;
	while (i < argc) {
		const char *arg = argv[i];
		size_t len = strlen(arg);

		/* check for a long option */
		if (g_str_has_prefix(arg, "--")) {
			char *name, *value;

			/* make sure we got an argument for the previous option */
			if( opt && opt->argument )
				option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);

			/* retrieve a option argument */
			if ((value=g_strrstr(arg+2, "="))) {
				*value = '\0';
				name = g_strdup(arg);
				*value = '=';
				value++;
			} else
				name = g_strdup(arg);

			/* check if the option exists */
			if( (opt=lookup_option(0, name+2)) == NULL )
				option_error(ERROR_UNKNOWN_OPTION, name, NULL);
			g_free(name);

			/* abort if we got an argument to the option and don't want one */
			if( value && opt->argument==NULL )
				option_error(ERROR_GOT_ARGUMENT, arg, value);

			/* execute option callback */
			if (value || opt->argument==NULL) {
				option_cb (opt->shortopt, value);
				opt = NULL;
			}
		}
		/* check for short options */
		else if (len>=2 && g_str_has_prefix(arg, "-")) {
			size_t j;

			for(j=1; j<len; j++) {
				/* make sure we got an argument for the previous option */
				if (opt && opt->argument)
					option_error(ERROR_MISSING_ARGUMENT,
						     opt->longopt, opt->argument);

				/* check if the option exists */
				if ((opt=lookup_option(arg[j], NULL)) == NULL)
					option_error(ERROR_UNKNOWN_OPTION, arg, NULL);

				/* if no option argument is needed execute callback */
				if (opt->argument == NULL) {
					option_cb (opt->shortopt, NULL);
					opt = NULL;
				}
			}
		} else {
			/* is this a option argument? */
			if (opt && opt->argument) {
				option_cb (opt->shortopt, arg);
				opt = NULL;
			} else
				option_error(ERROR_BAD_ARGUMENT, arg, NULL);
		}
		i++;
	}

	if (opt && opt->argument == NULL)
		option_cb (opt->shortopt, NULL);
	else if (opt && opt->argument)
		option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);
}

void
options_init(void)
{
	/* default option values */
	options.list_format = g_strdup(DEFAULT_LIST_FORMAT);
	options.status_format = g_strdup(DEFAULT_STATUS_FORMAT);
	options.screen_list = g_strsplit_set(DEFAULT_SCREEN_LIST, " ", 0);
#ifndef NCMPC_MINI
	options.scroll_sep = g_strdup(DEFAULT_SCROLL_SEP);
#endif
}

void
options_deinit(void)
{
	g_free(options.host);
	g_free(options.username);
	g_free(options.password);
	g_free(options.config_file);
	g_free(options.key_file);
	g_free(options.list_format);
	g_free(options.status_format);
	g_strfreev(options.screen_list);
#ifndef NCMPC_MINI
	g_free(options.xterm_title_format);
	g_free(options.scroll_sep);
#endif
}
