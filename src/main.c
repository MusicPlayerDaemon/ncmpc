/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"
#include "ncmpc.h"
#include "mpdclient.h"
#include "support.h"
#include "options.h"
#include "conf.h"
#include "command.h"
#include "lyrics.h"
#include "screen.h"
#include "screen_utils.h"
#include "strfsong.h"
#include "gcc.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BUFSIZE 1024

static const guint idle_interval = 500;

static mpdclient_t *mpd = NULL;
static gboolean connected = FALSE;
static GMainLoop *main_loop;
static guint reconnect_source_id, idle_source_id, update_source_id;

static const gchar *
error_msg(const gchar *msg)
{
	gchar *p;

	if ((p = strchr(msg, '}')) == NULL)
		return msg;

	while (p && *p && (*p=='}' || *p==' '))
		p++;

	return p;
}

static void
error_callback(mpd_unused mpdclient_t *c, gint error, const gchar *msg)
{
	error = error & 0xFF;
	D("Error [%d:%d]> \"%s\"\n", error, GET_ACK_ERROR_CODE(error), msg);
	switch (error) {
	case MPD_ERROR_CONNPORT:
	case MPD_ERROR_NORESPONSE:
		break;
	case MPD_ERROR_ACK:
		screen_status_printf("%s", error_msg(msg));
		screen_bell();
		break;
	default:
		screen_status_printf("%s", msg);
		screen_bell();
		doupdate();
		connected = FALSE;
	}
}

static void
update_xterm_title(void)
{
	static char title[BUFSIZE];
	char tmp[BUFSIZE];
	mpd_Status *status = NULL;
	mpd_Song *song = NULL;

	if (mpd) {
		status = mpd->status;
		song = mpd->song;
	}

	if (options.xterm_title_format && status && song &&
	    IS_PLAYING(status->state))
		strfsong(tmp, BUFSIZE, options.xterm_title_format, song);
	else
		g_strlcpy(tmp, PACKAGE " version " VERSION, BUFSIZE);

	if (strncmp(title, tmp, BUFSIZE)) {
		g_strlcpy(title, tmp, BUFSIZE);
		set_xterm_title("%s", title);
	}
}

static void
exit_and_cleanup(void)
{
	screen_exit();
	set_xterm_title("");
	printf("\n");

	if (mpd) {
		mpdclient_disconnect(mpd);
		mpdclient_free(mpd);
	}

	g_free(options.host);
	g_free(options.password);
	g_free(options.list_format);
	g_free(options.status_format);
	g_free(options.scroll_sep);
}

static void
catch_sigint(mpd_unused int sig)
{
	g_main_loop_quit(main_loop);
}


static void
catch_sigcont(mpd_unused int sig)
{
	D("catch_sigcont()\n");
#ifdef ENABLE_RAW_MODE
	reset_prog_mode(); /* restore tty modes */
	refresh();
#endif
	screen_resize();
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
timer_sigwinch(mpd_unused gpointer data)
{
	/* the following causes the screen to flicker.  There might be
	   better solutions, but I believe it isn't all that
	   important. */

	endwin();
	refresh();
	screen_resize();

	return FALSE;
}

static void
catch_sigwinch(mpd_unused int sig)
{
	if (timer_sigwinch_id != 0)
		g_source_remove(timer_sigwinch_id);

	timer_sigwinch_id = g_timeout_add(100, timer_sigwinch, NULL);
}

#ifndef NDEBUG
void 
D(const char *format, ...)
{
  if( options.debug )
    {
      gchar *msg;
      va_list ap;
  
      va_start(ap,format);
      msg = g_strdup_vprintf(format,ap);
      va_end(ap);
      fprintf(stderr, "%s", msg);
      g_free(msg);
    }
}
#endif

static gboolean
timer_mpd_update(gpointer data);

/**
 * This timer is installed when the connection to the MPD server is
 * broken.  It tries to recover by reconnecting periodically.
 */
static gboolean
timer_reconnect(mpd_unused gpointer data)
{
	int ret;

	if (connected)
		return FALSE;

	screen_status_printf(_("Connecting to %s...  [Press %s to abort]"),
			     options.host, get_key_names(CMD_QUIT,0) );
	doupdate();

	mpdclient_disconnect(mpd);
	ret = mpdclient_connect(mpd,
				options.host, options.port,
				1.5,
				options.password);
	if (ret != 0) {
		/* try again in 5 seconds */
		g_timeout_add(5000, timer_reconnect, NULL);
		return FALSE;
	}

	/* quit if mpd is pre 0.11.0 - song id not supported by mpd */
	if (MPD_VERSION_LT(mpd, 0, 11, 0)) {
		screen_status_printf(_("Error: MPD version %d.%d.%d is to old (0.11.0 needed).\n"),
				     mpd->connection->version[0],
				     mpd->connection->version[1],
				     mpd->connection->version[2]);
		mpdclient_disconnect(mpd);
		doupdate();

		/* try again after 30 seconds */
		g_timeout_add(30000, timer_reconnect, NULL);
		return FALSE;
	}

	screen_status_printf(_("Connected to %s!"), options.host);
	doupdate();

	connected = TRUE;

	/* update immediately */
	g_timeout_add(1, timer_mpd_update, GINT_TO_POINTER(FALSE));

	reconnect_source_id = 0;
	return FALSE;

}

static gboolean
timer_mpd_update(gpointer data)
{
	if (connected)
		mpdclient_update(mpd);
	else if (reconnect_source_id == 0)
		reconnect_source_id = g_timeout_add(1000, timer_reconnect,
						    NULL);

	if (options.enable_xterm_title)
		update_xterm_title();

	screen_update(mpd);

	return GPOINTER_TO_INT(data);
}

/**
 * This idle timer is invoked when the user hasn't typed a key for
 * 500ms.  It is used for delayed seeking.
 */
static gboolean
timer_idle(mpd_unused gpointer data)
{
	screen_idle(mpd);
	return TRUE;
}

static gboolean
keyboard_event(mpd_unused GIOChannel *source,
	       mpd_unused GIOCondition condition, mpd_unused gpointer data)
{
	command_t cmd;

	/* remove the idle timeout; add it later with fresh interval */
	g_source_remove(idle_source_id);

	if ((cmd=get_keyboard_command()) != CMD_NONE) {
		if (cmd == CMD_QUIT) {
			g_main_loop_quit(main_loop);
			return FALSE;
		}

		screen_cmd(mpd, cmd);

		if (cmd == CMD_VOLUME_UP || cmd == CMD_VOLUME_DOWN) {
			/* make sure we dont update the volume yet */
			g_source_remove(update_source_id);
			update_source_id = g_timeout_add((guint)(MPD_UPDATE_TIME * 1000),
							 timer_mpd_update,
							 GINT_TO_POINTER(TRUE));
		}
	}

	screen_update(mpd);

	idle_source_id = g_timeout_add(idle_interval, timer_idle, NULL);
	return TRUE;
}

/**
 * Check the configured key bindings for errors, and display a status
 * message every 10 seconds.
 */
static gboolean
timer_check_key_bindings(mpd_unused gpointer data)
{
	char buf[256];
	gboolean key_error;

	key_error = check_key_bindings(NULL, buf, sizeof(buf));
	if (!key_error)
		/* no error: disable this timer for the rest of this
		   process */
		return FALSE;

	screen_status_printf("%s", buf);
	doupdate();
	return TRUE;
}

int
main(int argc, const char *argv[])
{
	struct sigaction act;
	const char *charset = NULL;
	GIOChannel *keyboard_channel;

#ifdef HAVE_LOCALE_H
	/* time and date formatting */
	setlocale(LC_TIME,"");
	/* care about sorting order etc */
	setlocale(LC_COLLATE,"");
	/* charset */
	setlocale(LC_CTYPE,"");
	/* initialize charset conversions */
	charset_init(g_get_charset(&charset));
	D("charset: %s\n", charset);
#endif

	/* initialize i18n support */
#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, charset);
	textdomain(GETTEXT_PACKAGE);
#endif

	/* initialize options */
	options_init();

	/* parse command line options - 1 pass get configuration files */
	options_parse(argc, argv);

	/* read configuration */
	read_configuration(&options);

	/* check key bindings */
	check_key_bindings(NULL, NULL, 0);

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
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = catch_sigint;
	if (sigaction(SIGTERM, &act, NULL) < 0) {
		perror("sigaction()");
		exit(EXIT_FAILURE);
	}

	/* setup signal behavior - SIGCONT */
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = catch_sigcont;
	if (sigaction(SIGCONT, &act, NULL) < 0) {
		perror("sigaction(SIGCONT)");
		exit(EXIT_FAILURE);
	}

	/* setup signal behaviour - SIGHUP*/
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = catch_sigint;
	if (sigaction(SIGHUP, &act, NULL) < 0) {
		perror("sigaction(SIGHUP)");
		exit(EXIT_FAILURE);
	}

	/* setup SIGWINCH */

	act.sa_handler = catch_sigwinch;
	if (sigaction(SIGWINCH, &act, NULL) < 0) {
		perror("sigaction(SIGWINCH)");
		exit(EXIT_FAILURE);
	}

	ncurses_init();

	lyrics_init();

	/* create mpdclient instance */
	mpd = mpdclient_new();
	mpdclient_install_error_callback(mpd, error_callback);

	/* initialize curses */
	screen_init(mpd);

	/* the main loop */
	main_loop = g_main_loop_new(NULL, FALSE);

	/* watch out for keyboard input */
	keyboard_channel = g_io_channel_unix_new(STDIN_FILENO);
	g_io_add_watch(keyboard_channel, G_IO_IN, keyboard_event, NULL);

	/* attempt to connect */
	reconnect_source_id = g_timeout_add(1, timer_reconnect, NULL);

	update_source_id = g_timeout_add((guint)(MPD_UPDATE_TIME * 1000),
					 timer_mpd_update,
					 GINT_TO_POINTER(TRUE));
	g_timeout_add(10000, timer_check_key_bindings, NULL);
	idle_source_id = g_timeout_add(idle_interval, timer_idle, NULL);

	g_main_loop_run(main_loop);

	/* cleanup */

	g_main_loop_unref(main_loop);
	g_io_channel_unref(keyboard_channel);

	exit_and_cleanup();
}
