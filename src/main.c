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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>

#include "config.h"
#include "ncmpc.h"
#include "mpdclient.h"
#include "support.h"
#include "options.h"
#include "command.h"
#include "screen.h"
#include "conf.h"

static mpdclient_t   *mpd = NULL;
static gboolean connected = FALSE;
static GTimer      *timer = NULL;

static void
error_callback(mpdclient_t *c, int error, char *msg)
{
  D("error_callback> error=%d errorCode=%d errorAt=%d\n", 
    error, c->connection->errorCode, c->connection->errorAt);
  D("error_callback> \"%s\"\n", msg);
  switch(error)
    {
    case MPD_ERROR_ACK:
      screen_status_printf("%s", msg);
      break;
    default:
      screen_status_printf(_("Lost connection to %s"), options.host);
      connected = FALSE;
    }
  doupdate();
}

void
exit_and_cleanup(void)
{
  screen_exit();
  printf("\n");
  if( mpd )
    {
      mpdclient_disconnect(mpd);
      mpd = mpdclient_free(mpd);
    }
  g_free(options.host);
  g_free(options.password);
  if( timer )
    g_timer_destroy(timer);
}

void
catch_sigint( int sig )
{
  printf("\n%s\n", _("Exiting..."));
  exit(EXIT_SUCCESS);
}

int 
main(int argc, const char *argv[])
{
  options_t *options;
  struct sigaction act;
  const char *charset = NULL;

#ifdef HAVE_LOCALE_H
  /* time and date formatting */
  setlocale(LC_TIME,"");
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
  options = options_init();

  /* parse command line options - 1 pass get configuration files */
  options_parse(argc, argv);
  
  /* read configuration */
  read_configuration(options);
  
  /* check key bindings */
  if( check_key_bindings() )
    {
      fprintf(stderr, _("Confusing key bindings - exiting!\n"));
      exit(EXIT_FAILURE);
    }

  /* parse command line options - 2 pass */
  options_parse(argc, argv);

  /* setup signal behavior - SIGINT */
  sigemptyset( &act.sa_mask );
  act.sa_flags    = 0;
  act.sa_handler = catch_sigint;
  if( sigaction( SIGINT, &act, NULL )<0 )
    {
      perror("signal");
      exit(EXIT_FAILURE);
    }
  /* setup signal behavior - SIGTERM */
  sigemptyset( &act.sa_mask );
  act.sa_flags    = 0;
  act.sa_handler = catch_sigint;
  if( sigaction( SIGTERM, &act, NULL )<0 )
    {
      perror("sigaction()");
      exit(EXIT_FAILURE);
    }

  /* set xterm title */
  if( g_getenv("DISPLAY") )
    printf("%c]0;%s%c", '\033', PACKAGE " version " VERSION, '\007');

  /* install exit function */
  atexit(exit_and_cleanup);

  /* connect to our music player daemon */
  mpd = mpdclient_new();
  if( mpdclient_connect(mpd, 
			options->host, 
			options->port, 
			10.0, 
			options->password) )
    {
      exit(EXIT_FAILURE);
    }
  connected = TRUE;
  D("Connected to MPD version %d.%d.%d\n",
    mpd->connection->version[0],
    mpd->connection->version[1],
    mpd->connection->version[2]);

  /* initialize curses */
  screen_init(mpd);

  /* install error callback function */
  mpdclient_install_error_callback(mpd, error_callback);

  /* initialize timer */
  timer = g_timer_new();

  connected = TRUE;
  while( connected || options->reconnect )
    {
      static gdouble t = G_MAXDOUBLE;

      if( connected && (t>=MPD_UPDATE_TIME || mpd->need_update) )
	{
	  mpdclient_update(mpd);
	  g_timer_start(timer);
	}

      if( connected )
	{
	  command_t cmd;

	  screen_update(mpd);
	  if( (cmd=get_keyboard_command()) != CMD_NONE )
	    {
	      screen_cmd(mpd, cmd);
	      if( cmd==CMD_VOLUME_UP || cmd==CMD_VOLUME_DOWN)
		/* make shure we dont update the volume yet */
		g_timer_start(timer);
	    }
	  else
	    screen_idle(mpd);
	}
      else if( options->reconnect )
	{
	  screen_status_printf(_("Connecting to %s...  [Press %s to abort]"), 
			       options->host, get_key_names(CMD_QUIT,0) );
	  doupdate();

	  if( get_keyboard_command_with_timeout(MPD_RECONNECT_TIME)==CMD_QUIT)
	    exit(EXIT_SUCCESS);
	  
	  if( mpdclient_connect(mpd,
				options->host,
				options->port, 
				1.0,
				options->password) == 0 )
	    {
	      screen_status_printf(_("Connected to %s!"), options->host);
	      doupdate();
	      connected = TRUE;
	    }	  
	}

      t = g_timer_elapsed(timer, NULL);
    }
  exit(EXIT_FAILURE);
}
