/* 
 * (c) 2004 by Kalle Wallin (kaw@linux.se)
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
#include <string.h>
#include <ncurses.h>
#include <glib.h>
#include <popt.h>

#include "config.h"
#include "options.h"
#include "command.h"
#include "support.h"

options_t options;

static char *mpd_host     = NULL;
static char *mpd_password = NULL;
static char *config_file  = NULL;
static char *key_file     = NULL;

static struct poptOption optionsTable[] = {
#ifdef DEBUG
  { "debug",        'D', 0, 0, 'D', "Enable debug output." },
#endif
  { "version",      'V', 0, 0, 'V', "Display version information." },
  { "colors",       'c', 0, 0, 'c', "Enable colors." },
  { "no-colors",    'C', 0, 0, 'C', "Disable colors." },
  { "exit",         'e', 0, 0, 'e', "Exit on connection errors." },
  { "port",         'p', POPT_ARG_INT, &options.port, 0, 
    "Connect to server on port [" DEFAULT_PORT_STR "].", "PORT" },
  { "host",         'h', POPT_ARG_STRING, &mpd_host, 0, 
    "Connect to server [" DEFAULT_HOST "].", "HOSTNAME" },
  { "password",     'P', POPT_ARG_STRING, &mpd_password, 0, 
    "Connect with password.", "PASSWORD" },
  { "config",       'f', POPT_ARG_STRING, &config_file, 0, 
    "Read config from FILE." , "FILE" },
  { "key-file",     'k', POPT_ARG_STRING, &key_file, 0, 
    "Read key bindings from FILE." , "FILE" },

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

  mpd_host = NULL;
  mpd_password = NULL;
  config_file = NULL;
  key_file = NULL;
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

  if( mpd_host )
    {
      g_free(options.host);
      options.host = mpd_host;
    }
  if( mpd_password )
    {
      g_free(options.password);
      options.password = mpd_password;
    }
  if( config_file )
    {
      g_free(options.config_file);
      options.config_file = config_file;
    }
  if( key_file )
    {
      g_free(options.key_file);
      options.key_file = key_file;
    }

  poptFreeContext(optCon);
  return &options;
}

options_t *
options_init( void )
{
  const char *value;
  char *tmp;

  memset(&options, 0, sizeof(options_t));

  if( (value=g_getenv(MPD_HOST_ENV)) )
    options.host = g_strdup(value);
  else
    options.host = g_strdup(DEFAULT_HOST);
  if( (tmp=g_strstr_len(options.host, strlen(options.host), "@")) )
    {
      char *oldhost = options.host;
      *tmp  = '\0';
      options.password = locale_to_utf8(oldhost);
      options.host = g_strdup(tmp+1);
      g_free(oldhost);
    }

  if( (value=g_getenv(MPD_PORT_ENV)) )
    options.port = atoi(value);
  else
    options.port = DEFAULT_PORT;

  options.reconnect = 1;
  options.find_wrap = 1;
  options.wide_cursor = 1;

  return &options;
}


options_t *
options_get(void)
{
  return &options;
}
