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

#include "config.h"
#include "ncmpc.hxx"
#include "mpdclient.hxx"
#include "callbacks.hxx"
#include "charset.hxx"
#include "options.hxx"
#include "command.hxx"
#include "ncu.hxx"
#include "screen.hxx"
#include "screen_status.hxx"
#include "xterm_title.hxx"
#include "strfsong.hxx"
#include "i18n.h"
#include "player_command.hxx"
#include "keyboard.hxx"
#include "lirc.hxx"
#include "signals.hxx"

#ifndef NCMPC_MINI
#include "conf.hxx"
#endif

#ifdef ENABLE_LYRICS_SCREEN
#include "lyrics.hxx"
#endif

#include <mpd/client.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

/* time between mpd updates [ms] */
static const guint update_interval = 500;

#define BUFSIZE 1024

static struct mpdclient *mpd = nullptr;
static GMainLoop *main_loop;
static guint reconnect_source_id, update_source_id;

#ifndef NCMPC_MINI
static guint check_key_bindings_source_id;
#endif

ScreenManager screen;

#ifndef NCMPC_MINI
static void
update_xterm_title()
{
	const struct mpd_song *song = mpd->song;

	char tmp[BUFSIZE];
	const char *new_title = nullptr;
	if (options.xterm_title_format && mpd->playing && song)
		new_title = strfsong(tmp, BUFSIZE, options.xterm_title_format, song) > 0
			? tmp
			: nullptr;

	if (new_title == nullptr)
		new_title = PACKAGE " version " VERSION;

	static char title[BUFSIZE];
	if (strncmp(title, new_title, BUFSIZE)) {
		g_strlcpy(title, new_title, BUFSIZE);
		set_xterm_title(title);
	}
}
#endif

static gboolean
timer_mpd_update(gpointer data);

static void
enable_update_timer()
{
	if (update_source_id != 0)
		return;

	update_source_id = g_timeout_add(update_interval,
					 timer_mpd_update, nullptr);
}

static void
disable_update_timer()
{
	if (update_source_id == 0)
		return;

	g_source_remove(update_source_id);
	update_source_id = 0;
}

static bool
should_enable_update_timer()
{
	return mpd->playing;
}

static void
auto_update_timer()
{
	if (should_enable_update_timer())
		enable_update_timer();
	else
		disable_update_timer();
}

static void
do_mpd_update()
{
	if (mpd->IsConnected() &&
	    (mpd->events != 0 || mpd->playing))
		mpdclient_update(mpd);

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen.Update(*mpd);
	mpd->events = (enum mpd_idle)0;
}

/**
 * This timer is installed when the connection to the MPD server is
 * broken.  It tries to recover by reconnecting periodically.
 */
static gboolean
timer_reconnect(gcc_unused gpointer data)
{
	assert(mpdclient_is_dead(mpd));

	reconnect_source_id = 0;

	char *name = mpdclient_settings_name(mpd);
	screen_status_printf(_("Connecting to %s...  [Press %s to abort]"),
			     name, get_key_names(CMD_QUIT, false));
	g_free(name);
	doupdate();

	mpd->Connect();

	return false;
}

void
mpdclient_connected_callback()
{
	assert(reconnect_source_id == 0);

#ifndef NCMPC_MINI
	/* quit if mpd is pre 0.14 - song id not supported by mpd */
	struct mpd_connection *connection = mpdclient_get_connection(mpd);
	if (mpd_connection_cmp_server_version(connection, 0, 16, 0) < 0) {
		const unsigned *version =
			mpd_connection_get_server_version(connection);
		screen_status_printf(_("Error: MPD version %d.%d.%d is too old (%s needed)"),
				     version[0], version[1], version[2],
				     "0.16.0");
		mpd->Disconnect();
		doupdate();

		/* try again after 30 seconds */
		reconnect_source_id =
			g_timeout_add_seconds(30, timer_reconnect, nullptr);
		return;
	}
#endif

	screen_status_clear_message();
	doupdate();

	/* update immediately */
	mpd->events = (enum mpd_idle)MPD_IDLE_ALL;

	do_mpd_update();

	auto_update_timer();
}

void
mpdclient_failed_callback()
{
	assert(reconnect_source_id == 0);

	/* try again in 5 seconds */
	reconnect_source_id = g_timeout_add_seconds(5, timer_reconnect, nullptr);
}

void
mpdclient_lost_callback()
{
	assert(reconnect_source_id == 0);

	screen.Update(*mpd);

	reconnect_source_id = g_timeout_add_seconds(1, timer_reconnect, nullptr);
}

/**
 * This function is called by the gidle.c library when MPD sends us an
 * idle event (or when the connection dies).
 */
void
mpdclient_idle_callback(gcc_unused unsigned events)
{
#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen.Update(*mpd);
	auto_update_timer();
}

static gboolean
timer_mpd_update(gcc_unused gpointer data)
{
	do_mpd_update();

	if (should_enable_update_timer())
		return true;
	else {
		update_source_id = 0;
		return false;
	}
}

void begin_input_event()
{
}

void end_input_event()
{
	screen.Update(*mpd);
	mpd->events = (enum mpd_idle)0;

	auto_update_timer();
}

bool
do_input_event(command_t cmd)
{
	if (cmd == CMD_QUIT) {
		g_main_loop_quit(main_loop);
		return false;
	}

	screen.OnCommand(mpd, cmd);

	if (cmd == CMD_VOLUME_UP || cmd == CMD_VOLUME_DOWN)
		/* make sure we don't update the volume yet */
		disable_update_timer();

	return true;
}

#ifdef HAVE_GETMOUSE

void
do_mouse_event(int x, int y, mmask_t bstate)
{
	screen.OnMouse(mpd, x, y, bstate);
}

#endif

#ifndef NCMPC_MINI
/**
 * Check the configured key bindings for errors, and display a status
 * message every 10 seconds.
 */
static gboolean
timer_check_key_bindings(gcc_unused gpointer data)
{
	char buf[256];

	if (check_key_bindings(nullptr, buf, sizeof(buf))) {
		/* no error: disable this timer for the rest of this
		   process */
		check_key_bindings_source_id = 0;
		return false;
	}

#ifdef ENABLE_KEYDEF_SCREEN
	g_strchomp(buf);
	g_strlcat(buf, " (", sizeof(buf));
	/* to translators: a key was bound twice in the key editor,
	   and this is a hint for the user what to press to correct
	   that */
	char comment[64];
	g_snprintf(comment, sizeof(comment), _("press %s for the key editor"),
		   get_key_names(CMD_SCREEN_KEYDEF, false));
	g_strlcat(buf, comment, sizeof(buf));
	g_strlcat(buf, ")", sizeof(buf));
#endif

	screen_status_printf("%s", buf);

	doupdate();
	return true;
}
#endif

int
main(int argc, const char *argv[])
{
#ifdef ENABLE_LOCALE
#ifndef ENABLE_NLS
	gcc_unused
#endif
	const char *charset = nullptr;
	/* time and date formatting */
	setlocale(LC_TIME,"");
	/* care about sorting order etc */
	setlocale(LC_COLLATE,"");
	/* charset */
	setlocale(LC_CTYPE,"");
	/* initialize charset conversions */
	charset = charset_init();

	/* initialize i18n support */
#endif

#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
#ifdef ENABLE_LOCALE
	bind_textdomain_codeset(GETTEXT_PACKAGE, charset);
#endif
	textdomain(GETTEXT_PACKAGE);
#endif

	/* initialize options */
	options_init();

	/* parse command line options - 1 pass get configuration files */
	options_parse(argc, argv);

#ifndef NCMPC_MINI
	/* read configuration */
	read_configuration();

	/* check key bindings */
	check_key_bindings(nullptr, nullptr, 0);
#endif

	/* parse command line options - 2 pass */
	options_parse(argc, argv);

	ncu_init();

#ifdef ENABLE_LYRICS_SCREEN
	lyrics_init();
#endif

	/* create mpdclient instance */
	mpd = mpdclient_new(options.host, options.port,
			    options.timeout_ms,
			    options.password);

	/* initialize curses */
	screen.Init(mpd);

	/* the main loop */
	main_loop = g_main_loop_new(nullptr, false);

	/* watch out for keyboard input */
	keyboard_init(screen.main_window.w);

	/* watch out for lirc input */
	ncmpc_lirc_init();

	signals_init(main_loop, screen);

	/* attempt to connect */
	reconnect_source_id = g_idle_add(timer_reconnect, nullptr);

	auto_update_timer();

#ifndef NCMPC_MINI
	check_key_bindings_source_id =
		g_timeout_add_seconds(10, timer_check_key_bindings, nullptr);
#endif

	screen.Paint(true);

	g_main_loop_run(main_loop);
	g_main_loop_unref(main_loop);

	/* cleanup */

	cancel_seek_timer();

	disable_update_timer();

	if (reconnect_source_id != 0)
		g_source_remove(reconnect_source_id);

#ifndef NCMPC_MINI
	if (check_key_bindings_source_id != 0)
		g_source_remove(check_key_bindings_source_id);
#endif

	signals_deinit();
	ncmpc_lirc_deinit();

	screen.Exit();
#ifndef NCMPC_MINI
	set_xterm_title("");
#endif
	printf("\n");

	mpdclient_free(mpd);

	ncu_deinit();
	options_deinit();

	return 0;
}
