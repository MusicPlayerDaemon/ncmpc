#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <glib.h>
#include <popt.h>

#include "config.h"
#include "options.h"
#include "command.h"

options_t options;

static struct poptOption optionsTable[] = {
#ifdef DEBUG
  { "debug",        'D', 0, 0, 'D', "Enable debug output." },
#endif
  { "version",      'V', 0, 0, 'V', "Display version information." },
  { "keys",         'k', 0, 0, 'k', "Display key bindings." },
  { "colors",       'c', 0, 0, 'c', "Enable colors." },
  { "no-colors",    'C', 0, 0, 'C', "Disable colors." },
  { "exit",         'e', 0, 0, 'e', "Exit on connection errors." },
  { "port",         'p', POPT_ARG_INT, &options.port, 0, 
    "Connect to server on port [" DEFAULT_PORT_STR "].", "PORT" },
  { "host",         'h', POPT_ARG_STRING, &options.host, 0, 
    "Connect to server [" DEFAULT_HOST "].", "HOSTNAME" },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0 }
};

static void
usage(poptContext optCon, int exitcode, char *error, char *addl) 
{
  poptPrintUsage(optCon, stderr, 0);
  if (error) 
    fprintf(stderr, "%s: %s0", error, addl);
  exit(exitcode);
}

options_t *
options_parse( int argc, const char **argv)
{
  int c;
  poptContext optCon;   /* context for parsing command-line options */

  optCon = poptGetContext(NULL, argc, argv, optionsTable, 0);
  while ((c = poptGetNextOpt(optCon)) >= 0) 
    {
      switch (c) 
	{
#ifdef DEBUG
	case 'D':
	  options.debug = 1;
	  break;
#endif
	case 'c':
	  options.enable_colors = 1;
	  break;
	case 'C':
	  options.enable_colors = 0;
	  break;
	case 'V':
	  printf("Version " VERSION "\n");
	  exit(EXIT_SUCCESS);
	case 'k':
	  command_dump_keys();
	  exit(EXIT_SUCCESS);
	case 'e':
	  options.reconnect = 0;
	  break;
	default:
	  fprintf(stderr, "%s: %s\n",
		  poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
		  poptStrerror(c));
	  poptFreeContext(optCon);
	  exit(EXIT_FAILURE);
	  break;
	}
    }
  if (c < -1) 
    {
      /* an error occurred during option processing */
      fprintf(stderr, "%s: %s\n",
	      poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
	      poptStrerror(c));
      poptFreeContext(optCon);
      exit(EXIT_FAILURE);
    }

  poptFreeContext(optCon);
  return &options;
}

options_t *
options_init( void )
{
  char *value;

  memset(&options, 0, sizeof(options_t));
  if( (value=getenv(MPD_HOST_ENV)) )
    options.host = g_strdup(value);
  else
    options.host = g_strdup(DEFAULT_HOST);
  if( (value=getenv(MPD_PORT_ENV)) )
    options.port = atoi(value);
  else
    options.port = DEFAULT_PORT;

  options.reconnect = 1;
  options.find_wrap = 1;

  options.bg_color       = COLOR_BLACK;
  options.title_color    = COLOR_BLUE;
  options.line_color     = COLOR_GREEN;
  options.list_color     = COLOR_YELLOW;
  options.progress_color = COLOR_GREEN;
  options.status_color   = COLOR_RED;
  options.alert_color    = COLOR_MAGENTA;;

  return &options;
}


options_t *
options_get(void)
{
  return &options;
}
