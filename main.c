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


static mpd_client_t *mpc = NULL;


void
exit_and_cleanup(void)
{
  screen_exit();
  charset_close();
  if( mpc )
    {
      if( mpc_error(mpc) )
	fprintf(stderr,"Error: %s\n", mpc_error_str(mpc));
      mpc_close(mpc);
    }
  g_free(options.host);
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
  int counter, connected;

  /* initialize options */
  options = options_init();
  
  /* read configuration */
  read_rc_file(NULL, options);

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
  if( getenv("DISPLAY") )
    printf("%c]0;%s%c", '\033', PACKAGE " v" VERSION, '\007');

  /* install exit function */
  atexit(exit_and_cleanup);

  /* connect to our music player daemon */
  mpc = mpc_connect(options->host, options->port);
  if( mpc_error(mpc) )
    exit(EXIT_FAILURE);

  /* initialize curses */
  screen_init();
  
  counter=0;
  connected=1;
  while( connected || options->reconnect )
    {
      command_t cmd;

      if( connected && counter==0  )
	{
	  mpc_update(mpc);
	  if( mpc_error(mpc) )
	    {
	      connected=0;
	      screen_status_printf("Lost connection to %s", options->host);
	      doupdate();
	      mpd_closeConnection(mpc->connection);
	      mpc->connection = NULL;
	    }
	  else
	    mpd_finishCommand(mpc->connection);
	  counter=10;
	}


      if( connected )
	{
	  screen_update(mpc);
	  if( (cmd=get_keyboard_command()) != CMD_NONE )
	    {
	      screen_cmd(mpc, cmd);
	      if( cmd==CMD_VOLUME_UP || cmd==CMD_VOLUME_DOWN)
		counter=10; /* make shure we dont update the volume yet */
	      else
		counter=0;
	    }
	}
      else if( options->reconnect )
	{
	  sleep(3);
	  screen_status_printf("Connecting to %s...  [Press Ctrl-C to abort]", 
			       options->host);
	  if( mpc_reconnect(mpc, options->host, options->port) == 0 )
	    {
	      screen_status_printf("Connected to %s!", options->host);
	      connected=1;
	      counter=0;
	    }
	  doupdate();
	}

      if( counter>0 )
	counter--;

    }
  exit(EXIT_FAILURE);
}
