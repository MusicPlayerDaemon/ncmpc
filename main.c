#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>

#include "config.h"
#include "libmpdclient.h"
#include "support.h"
#include "mpc.h"
#include "options.h"
#include "command.h"
#include "screen.h"
#include "conf.h"

/* time in seconds between mpd updates (double) */
#define MPD_UPDATE_TIME        0.5

/* timout in seconds before trying to reconnect (int) */
#define MPD_RECONNECT_TIMEOUT  3


static mpd_client_t *mpc = NULL;
static GTimer       *timer = NULL;

void
exit_and_cleanup(void)
{
  screen_exit();
  printf("\n");
  charset_close();
  if( mpc )
    {
      if( mpc_error(mpc) )
	fprintf(stderr,"Error: %s\n", mpc_error_str(mpc));
      mpc_close(mpc);
    }
  g_free(options.host);
  g_free(options.password);
  if( timer )
    g_timer_destroy(timer);
}

void
catch_sigint( int sig )
{
  printf( "\nExiting...\n");
  exit(EXIT_SUCCESS);
}

int 
main(int argc, const char *argv[])
{
  options_t *options;
  struct sigaction act;
  gboolean connected;

  /* initialize options */
  options = options_init();
  
  /* read configuration */
  read_configuration(options);
  
  /* check key bindings */
  if( check_key_bindings() )
    {
      fprintf(stderr, "Confusing key bindings - exiting!\n");
      exit(EXIT_FAILURE);
    }

  /* parse command line options */
  options_parse(argc, argv);

  /* initialize local charset */
  if( charset_init() )
    exit(EXIT_FAILURE);

  /* setup signal behavior - SIGINT */
  sigemptyset( &act.sa_mask );
  act.sa_flags    = 0;
  act.sa_handler = catch_sigint;
  if( sigaction( SIGINT, &act, NULL )<0 )
    {
      perror("signal");
      exit(EXIT_FAILURE);
    }
  /* setup signal behavior - SIGWINCH  */
  sigemptyset( &act.sa_mask );
  act.sa_flags    = 0;
  act.sa_handler = screen_resized;
  if( sigaction( SIGWINCH, &act, NULL )<0 )
    {
      perror("sigaction()");
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
  mpc = mpc_connect(options->host, options->port, options->password);
  if( mpc_error(mpc) )
    exit(EXIT_FAILURE);

  /* initialize curses */
  screen_init();

  /* initialize timer */
  timer = g_timer_new();

  connected = TRUE;
  while( connected || options->reconnect )
    {
      static gdouble t = G_MAXDOUBLE;

      if( connected && t>=MPD_UPDATE_TIME )
	{
	  mpc_update(mpc);
	  if( mpc_error(mpc) == MPD_ERROR_ACK )
	    {
	      screen_status_printf("%s", mpc_error_str(mpc));
	      mpd_clearError(mpc->connection);
	      mpd_finishCommand(mpc->connection);
	    }
	  else if( mpc_error(mpc) )
	    {
	      screen_status_printf("Lost connection to %s", options->host);
	      connected = FALSE;	 
	      doupdate();
	      mpd_clearError(mpc->connection);
	      mpd_closeConnection(mpc->connection);
	      mpc->connection = NULL;
	    }
	  else	
	    mpd_finishCommand(mpc->connection);
	  g_timer_start(timer);
	}

      if( connected )
	{
	  command_t cmd;

	  screen_update(mpc);
	  if( (cmd=get_keyboard_command()) != CMD_NONE )
	    {
	      screen_cmd(mpc, cmd);
	      if( cmd==CMD_VOLUME_UP || cmd==CMD_VOLUME_DOWN)
		/* make shure we dont update the volume yet */
		g_timer_start(timer);
	    }
	}
      else if( options->reconnect )
	{
	  sleep(MPD_RECONNECT_TIMEOUT);
	  screen_status_printf("Connecting to %s...  [Press Ctrl-C to abort]", 
			       options->host);
	  if( mpc_reconnect(mpc, 
			    options->host, 
			    options->port, 
			    options->password) == 0 )
	    {
	      screen_status_printf("Connected to %s!", options->host);
	      connected = TRUE;
	    }
	  doupdate();
	}

      t = g_timer_elapsed(timer, NULL);
    }

  exit(EXIT_FAILURE);
}
