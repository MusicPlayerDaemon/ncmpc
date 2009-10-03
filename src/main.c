/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
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

#include "config.h"
#include "ncmpc.h"
#include "mpdclient.h"
#include "gidle.h"
#include "charset.h"
#include "options.h"
#include "command.h"
#include "ncu.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_message.h"
#include "strfsong.h"
#include "i18n.h"
#include "player_command.h"

#ifndef NCMPC_MINI
#include "conf.h"
#endif

#ifdef ENABLE_LYRICS_SCREEN
#include "lyrics.h"
#endif

#ifdef ENABLE_LIRC
#include "lirc.h"
#endif

#include <mpd/client.h>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

/* time between mpd updates [s] */
static const guint update_interval = 500;

#define BUFSIZE 1024

static struct mpdclient *mpd = NULL;
static GMainLoop *main_loop;
static guint reconnect_source_id, update_source_id;

#ifndef NCMPC_MINI
static guint check_key_bindings_source_id;
#endif

#ifndef NCMPC_MINI
static void
update_xterm_title(void)
{
	static char title[BUFSIZE];
	char tmp[BUFSIZE];
	struct mpd_status *status = NULL;
	const struct mpd_song *song = NULL;

	if (mpd) {
		status = mpd->status;
		song = mpd->song;
	}

	if (options.xterm_title_format && status && song &&
	    mpd_status_get_state(status) == MPD_STATE_PLAY)
		strfsong(tmp, BUFSIZE, options.xterm_title_format, song);
	else
		g_strlcpy(tmp, PACKAGE " version " VERSION, BUFSIZE);

	if (strncmp(title, tmp, BUFSIZE)) {
		g_strlcpy(title, tmp, BUFSIZE);
		set_xterm_title("%s", title);
	}
}
#endif

static void
exit_and_cleanup(void)
{
	screen_exit();
#ifndef NCMPC_MINI
	set_xterm_title("");
#endif
	printf("\n");

	if (mpd) {
		mpdclient_disconnect(mpd);
		mpdclient_free(mpd);
	}
}

static void
catch_sigint(G_GNUC_UNUSED int sig)
{
	g_main_loop_quit(main_loop);
}


static void
catch_sigcont(G_GNUC_UNUSED int sig)
{
	screen_resize(mpd);
}

void
sigstop(void)
{
  def_prog_mode();  /* save the tty modes */
  endwin();         /* end curses mode temporarily */
  kill(0, SIGSTOP); /* issue SIGSTOP */
}

static guint timer_sigwinch_id;

static gboolean
timer_sigwinch(G_GNUC_UNUSED gpointer data)
{
	/* the following causes the screen to flicker.  There might be
	   better solutions, but I believe it isn't all that
	   important. */

	endwin();
	refresh();
	screen_resize(mpd);

	return FALSE;
}

static void
catch_sigwinch(G_GNUC_UNUSED int sig)
{
	if (timer_sigwinch_id != 0)
		g_source_remove(timer_sigwinch_id);

	timer_sigwinch_id = g_timeout_add(100, timer_sigwinch, NULL);
}

static void
idle_callback(enum mpd_error error,
	      G_GNUC_UNUSED enum mpd_server_error server_error,
	      const char *message, enum mpd_idle events,
	      G_GNUC_UNUSED void *ctx);

static gboolean
timer_mpd_update(gpointer data);

static void
enable_update_timer(void)
{
	if (update_source_id != 0)
		return;

	update_source_id = g_timeout_add(update_interval,
					 timer_mpd_update, NULL);
}

static void
disable_update_timer(void)
{
	if (update_source_id == 0)
		return;

	g_source_remove(update_source_id);
	update_source_id = 0;
}

static bool
should_enable_update_timer(void)
{
	return (mpdclient_is_connected(mpd) &&
		(mpd->source == NULL || /* "idle" not supported */
		 (mpd->status != NULL &&
		  mpd_status_get_state(mpd->status) == MPD_STATE_PLAY)))
#ifndef NCMPC_MINI
		|| options.display_time
#endif
		;
}

static void
auto_update_timer(void)
{
	if (should_enable_update_timer())
		enable_update_timer();
	else
		disable_update_timer();
}

static void
check_reconnect(void);

static void
do_mpd_update(void)
{
	if (mpdclient_is_connected(mpd))
		mpdclient_update(mpd);

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen_update(mpd);
	mpd->events = 0;

	mpdclient_put_connection(mpd);
	check_reconnect();
}

/**
 * This timer is installed when the connection to the MPD server is
 * broken.  It tries to recover by reconnecting periodically.
 */
static gboolean
timer_reconnect(G_GNUC_UNUSED gpointer data)
{
	bool success;
	struct mpd_connection *connection;

	assert(!mpdclient_is_connected(mpd));

	reconnect_source_id = 0;

	screen_status_printf(_("Connecting to %s...  [Press %s to abort]"),
			     options.host, get_key_names(CMD_QUIT,0) );
	doupdate();

	mpdclient_disconnect(mpd);
	success = mpdclient_connect(mpd,
				    options.host, options.port,
				    1.5,
				    options.password);
	if (!success) {
		/* try again in 5 seconds */
		reconnect_source_id = g_timeout_add(5000,
						    timer_reconnect, NULL);
		return FALSE;
	}

	connection = mpdclient_get_connection(mpd);

#ifndef NCMPC_MINI
	/* quit if mpd is pre 0.11.0 - song id not supported by mpd */
	if (mpd_connection_cmp_server_version(connection, 0, 12, 0) < 0) {
		const unsigned *version =
			mpd_connection_get_server_version(connection);
		screen_status_printf(_("Error: MPD version %d.%d.%d is to old (%s needed)"),
				     version[0], version[1], version[2],
				     "0.12.0");
		mpdclient_disconnect(mpd);
		doupdate();

		/* try again after 30 seconds */
		reconnect_source_id = g_timeout_add(30000,
						    timer_reconnect, NULL);
		return FALSE;
	}
#endif

	if (mpd_connection_cmp_server_version(connection,
					      0, 14, 0) >= 0)
		mpd->source = mpd_glib_new(connection,
					   idle_callback, mpd);

	screen_status_printf(_("Connected to %s"),
			     options.host != NULL
			     ? options.host : "localhost");
	doupdate();

	/* update immediately */
	mpd->events = MPD_IDLE_DATABASE|MPD_IDLE_STORED_PLAYLIST|
		MPD_IDLE_QUEUE|MPD_IDLE_PLAYER|MPD_IDLE_MIXER|MPD_IDLE_OUTPUT|
		MPD_IDLE_OPTIONS|MPD_IDLE_UPDATE;

	do_mpd_update();

	auto_update_timer();

	return FALSE;
}

static void
check_reconnect(void)
{
	if (!mpdclient_is_connected(mpd) && reconnect_source_id == 0)
		/* reconnect when the connection is lost */
		reconnect_source_id = g_timeout_add(1000, timer_reconnect,
						    NULL);
}

/**
 * This function is called by the gidle.c library when MPD sends us an
 * idle event (or when the connectiond dies).
 */
static void
idle_callback(enum mpd_error error, enum mpd_server_error server_error,
	      const char *message, enum mpd_idle events,
	      void *ctx)
{
	struct mpdclient *c = ctx;
	struct mpd_connection *connection;

	c->idle = false;

	connection = mpdclient_get_connection(c);
	assert(connection != NULL);

	if (error != MPD_ERROR_SUCCESS) {
		char *allocated;

		if (error == MPD_ERROR_SERVER &&
		    server_error == MPD_SERVER_ERROR_UNKNOWN_CMD) {
			/* the "idle" command is not supported - fall
			   back to timer based polling */
			mpd_glib_free(c->source);
			c->source = NULL;
			auto_update_timer();
			return;
		}

		if (error == MPD_ERROR_SERVER)
			message = allocated = utf8_to_locale(message);
		else
			allocated = NULL;
		screen_status_message(message);
		g_free(allocated);
		screen_bell();
		doupdate();

		mpdclient_disconnect(c);
		reconnect_source_id = g_timeout_add(1000, timer_reconnect,
						    NULL);
		return;
	}

	c->events |= events;
	mpdclient_update(c);

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen_update(mpd);
	c->events = 0;

	mpdclient_put_connection(c);
	check_reconnect();
	auto_update_timer();
}

static gboolean
timer_mpd_update(G_GNUC_UNUSED gpointer data)
{
	do_mpd_update();

	if (should_enable_update_timer())
		return true;
	else {
		update_source_id = 0;
		return false;
	}
}

void begin_input_event(void)
{
}

void end_input_event(void)
{
	screen_update(mpd);
	mpd->events = 0;

	mpdclient_put_connection(mpd);
	check_reconnect();
	auto_update_timer();
}

int do_input_event(command_t cmd)
{
	if (cmd == CMD_QUIT) {
		g_main_loop_quit(main_loop);
		return -1;
	}

	screen_cmd(mpd, cmd);

	if (cmd == CMD_VOLUME_UP || cmd == CMD_VOLUME_DOWN)
		/* make sure we don't update the volume yet */
		disable_update_timer();

	return 0;
}

static gboolean
keyboard_event(G_GNUC_UNUSED GIOChannel *source,
	       G_GNUC_UNUSED GIOCondition condition,
	       G_GNUC_UNUSED gpointer data)
{
	command_t cmd;

	begin_input_event();

	if ((cmd=get_keyboard_command()) != CMD_NONE)
		if (do_input_event(cmd) != 0)
			return FALSE;

	end_input_event();
	return TRUE;
}

#ifndef NCMPC_MINI
/**
 * Check the configured key bindings for errors, and display a status
 * message every 10 seconds.
 */
static gboolean
timer_check_key_bindings(G_GNUC_UNUSED gpointer data)
{
	char buf[256];
#ifdef ENABLE_KEYDEF_SCREEN
	char comment[64];
#endif
	gboolean key_error;

	key_error = check_key_bindings(NULL, buf, sizeof(buf));
	if (!key_error) {
		/* no error: disable this timer for the rest of this
		   process */
		check_key_bindings_source_id = 0;
		return FALSE;
	}

#ifdef ENABLE_KEYDEF_SCREEN
	g_strchomp(buf);
	g_strlcat(buf, " (", sizeof(buf));
	/* to translators: a key was bound twice in the key editor,
	   and this is a hint for the user what to press to correct
	   that */
	g_snprintf(comment, sizeof(comment), _("press %s for the key editor"),
		   get_key_names(CMD_SCREEN_KEYDEF, 0));
	g_strlcat(buf, comment, sizeof(buf));
	g_strlcat(buf, ")", sizeof(buf));
#endif

	screen_status_printf("%s", buf);

	doupdate();
	return TRUE;
}
#endif

int
main(int argc, const char *argv[])
{
	struct sigaction act;
#ifdef ENABLE_LOCALE
	const char *charset = NULL;
#endif
	GIOChannel *keyboard_channel;
#ifdef ENABLE_LIRC
	int lirc_socket;
	GIOChannel *lirc_channel = NULL;
#endif

#ifdef ENABLE_LOCALE
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
	check_key_bindings(NULL, NULL, 0);
#endif

	/* parse command line options - 2 pass */
	options_parse(argc, argv);

	/* setup signal behavior - SIGINT */
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = catch_sigint;
	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	/* setup signal behavior - SIGTERM */

	act.sa_handler = catch_sigint;
	if (sigaction(SIGTERM, &act, NULL) < 0) {
		perror("sigaction()");
		exit(EXIT_FAILURE);
	}

	/* setup signal behavior - SIGCONT */

	act.sa_handler = catch_sigcont;
	if (sigaction(SIGCONT, &act, NULL) < 0) {
		perror("sigaction(SIGCONT)");
		exit(EXIT_FAILURE);
	}

	/* setup signal behaviour - SIGHUP*/

	act.sa_handler = catch_sigint;
	if (sigaction(SIGHUP, &act, NULL) < 0) {
		perror("sigaction(SIGHUP)");
		exit(EXIT_FAILURE);
	}

	/* setup SIGWINCH */

	act.sa_flags = SA_RESTART;
	act.sa_handler = catch_sigwinch;
	if (sigaction(SIGWINCH, &act, NULL) < 0) {
		perror("sigaction(SIGWINCH)");
		exit(EXIT_FAILURE);
	}

	/* ignore SIGPIPE */

	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) < 0) {
		perror("sigaction(SIGPIPE)");
		exit(EXIT_FAILURE);
	}

	ncu_init();

#ifdef ENABLE_LYRICS_SCREEN
	lyrics_init();
#endif

	/* create mpdclient instance */
	mpd = mpdclient_new();

	/* initialize curses */
	screen_init(mpd);

	/* the main loop */
	main_loop = g_main_loop_new(NULL, FALSE);

	/* watch out for keyboard input */
	keyboard_channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(keyboard_channel, G_IO_IN, keyboard_event, NULL);

#ifdef ENABLE_LIRC
	/* watch out for lirc input */
	lirc_socket = ncmpc_lirc_open();
	if (lirc_socket >= 0) {
		lirc_channel = g_io_channel_unix_new(lirc_socket);
		g_io_add_watch(lirc_channel, G_IO_IN, lirc_event, NULL);
	}
#endif

	/* attempt to connect */
	reconnect_source_id = g_timeout_add(1, timer_reconnect, NULL);

	auto_update_timer();

#ifndef NCMPC_MINI
	check_key_bindings_source_id = g_timeout_add(10000, timer_check_key_bindings, NULL);
#endif

	screen_paint(mpd);

	g_main_loop_run(main_loop);

	/* cleanup */

	cancel_seek_timer();

	disable_update_timer();

	if (reconnect_source_id != 0)
		g_source_remove(reconnect_source_id);

#ifndef NCMPC_MINI
	if (check_key_bindings_source_id != 0)
		g_source_remove(check_key_bindings_source_id);
#endif

	g_main_loop_unref(main_loop);
	g_io_channel_unref(keyboard_channel);

#ifdef ENABLE_LIRC
	if (lirc_socket >= 0)
		g_io_channel_unref(lirc_channel);
	ncmpc_lirc_close();
#endif

	exit_and_cleanup();

#ifdef ENABLE_LYRICS_SCREEN
	lyrics_deinit();
#endif

	ncu_deinit();
	options_deinit();

	return 0;
}
