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
#include "src_lyrics.h"
#include "screen.h"
#include "screen_utils.h"
#include "strfsong.h"
#include "gcc.h"

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define BUFSIZE 1024

static mpdclient_t   *mpd = NULL;
static gboolean connected = FALSE;
static GTimer      *timer = NULL;

static const gchar *
error_msg(const gchar *msg)
{
  gchar *p;

  if( (p=strchr(msg, '}' )) == NULL )
    return msg;
  while( p && *p && (*p=='}' || *p==' ') )
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

  if( mpd )
    {
      status = mpd->status;
      song = mpd->song;
    }

  if(options.xterm_title_format && status && song && IS_PLAYING(status->state))
    {
      strfsong(tmp, BUFSIZE, options.xterm_title_format, song);
    }
  else
    g_strlcpy(tmp, PACKAGE " version " VERSION, BUFSIZE);

  if( strncmp(title,tmp,BUFSIZE) )
    {
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
  if( mpd )
    {
      mpdclient_disconnect(mpd);
      mpd = mpdclient_free(mpd);
    }
  g_free(options.host);
  g_free(options.password);
  g_free(options.list_format);
  g_free(options.status_format);
  g_free(options.scroll_sep);
  if( timer )
    g_timer_destroy(timer);
}

static void
catch_sigint(mpd_unused int sig)
{
	printf("\n%s\n", _("Exiting..."));
	exit(EXIT_SUCCESS);
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

int
main(int argc, const char *argv[])
{
	struct sigaction act;
	const char *charset = NULL;
	gboolean key_error;

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
	key_error = check_key_bindings(NULL, NULL, 0);

	/* parse command line options - 2 pass */
	options_parse(argc, argv);

	/* setup signal behavior - SIGINT */
	sigemptyset( &act.sa_mask );
	act.sa_flags    = 0;
	act.sa_handler = catch_sigint;
	if( sigaction(SIGINT, &act, NULL)<0 ) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	/* setup signal behavior - SIGTERM */
	sigemptyset( &act.sa_mask );
	act.sa_flags    = 0;
	act.sa_handler = catch_sigint;
	if( sigaction(SIGTERM, &act, NULL)<0 ) {
		perror("sigaction()");
		exit(EXIT_FAILURE);
	}

	/* setup signal behavior - SIGCONT */
	sigemptyset( &act.sa_mask );
	act.sa_flags    = 0;
	act.sa_handler = catch_sigcont;
	if( sigaction(SIGCONT, &act, NULL)<0 ) {
		perror("sigaction(SIGCONT)");
		exit(EXIT_FAILURE);
	}

	/* setup signal behaviour - SIGHUP*/
	sigemptyset( &act.sa_mask );
	act.sa_flags    = 0;
	act.sa_handler = catch_sigint;
	if( sigaction(SIGHUP, &act, NULL)<0 ) {
		perror("sigaction(SIGHUP)");
		exit(EXIT_FAILURE);
	}

	/* install exit function */
	atexit(exit_and_cleanup);

	ncurses_init();

	src_lyr_init ();

	/* connect to our music player daemon */
	mpd = mpdclient_new();

	if (mpdclient_connect(mpd,
			      options.host,
			      options.port,
			      10.0,
			      options.password))
		exit(EXIT_FAILURE);

	/* if no password is used, but the mpd wants one, the connection
	   might be established but no status information is avaiable */
	mpdclient_update(mpd);
	if (!mpd->status)
		screen_auth(mpd);

	if (!mpd->status)
		exit(EXIT_FAILURE);

	connected = TRUE;
	D("Connected to MPD version %d.%d.%d\n",
	  mpd->connection->version[0],
	  mpd->connection->version[1],
	  mpd->connection->version[2]);

	/* quit if mpd is pre 0.11.0 - song id not supported by mpd */
	if( MPD_VERSION_LT(mpd, 0,11,0) ) {
		fprintf(stderr,
			_("Error: MPD version %d.%d.%d is to old (0.11.0 needed).\n"),
			mpd->connection->version[0],
			mpd->connection->version[1],
			mpd->connection->version[2]);
		exit(EXIT_FAILURE);
	}

	/* initialize curses */
	screen_init(mpd);
	/* install error callback function */
	mpdclient_install_error_callback(mpd, error_callback);

	/* initialize timer */
	timer = g_timer_new();

	connected = TRUE;
	while (connected || options.reconnect) {
		static gdouble t = G_MAXDOUBLE;

		if (key_error) {
			char buf[BUFSIZE];

			key_error=check_key_bindings(NULL, buf, BUFSIZE);
			screen_status_printf("%s", buf);
		}

		if (connected && (t >= MPD_UPDATE_TIME || mpd->need_update)) {
			mpdclient_update(mpd);
			g_timer_start(timer);
		}

		if (connected) {
			command_t cmd;

			screen_update(mpd);
			if ((cmd=get_keyboard_command()) != CMD_NONE) {
				screen_cmd(mpd, cmd);
				if (cmd == CMD_VOLUME_UP || cmd == CMD_VOLUME_DOWN)
					/* make shure we dont update the volume yet */
					g_timer_start(timer);
			} else
				screen_idle(mpd);
		} else if (options.reconnect) {
			screen_status_printf(_("Connecting to %s...  [Press %s to abort]"),
					     options.host, get_key_names(CMD_QUIT,0) );

			if (get_keyboard_command_with_timeout(MPD_RECONNECT_TIME) == CMD_QUIT)
				exit(EXIT_SUCCESS);

			if (mpdclient_connect(mpd,
					      options.host, options.port,
					      1.5,
					      options.password) == 0) {
				screen_status_printf(_("Connected to %s!"), options.host);
				connected = TRUE;
			}

			doupdate();
		}

		if (options.enable_xterm_title)
			update_xterm_title();

		t = g_timer_elapsed(timer, NULL);
	}
	exit(EXIT_FAILURE);
}
