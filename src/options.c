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
#include <string.h>
#include <ncurses.h>
#include <glib.h>

#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "options.h"
#include "command.h"
#include "conf.h"

#define MAX_LONGOPT_LENGTH 32

#define ERROR_UNKNOWN_OPTION    0x01
#define ERROR_BAD_ARGUMENT      0x02
#define ERROR_GOT_ARGUMENT      0x03
#define ERROR_MISSING_ARGUMENT  0x04

 

typedef struct
{
  int  shortopt;
  char *longopt;
  char *argument;
  char *descrition;
} arg_opt_t;


typedef void (*option_callback_fn_t)   (int c, char *arg);


options_t options;
 
static arg_opt_t option_table[] = {
  { '?', "help",     NULL,   "Show this help message" },
  { 'V', "version",  NULL,   "Display version information" },
  { 'c', "colors",   NULL,   "Enable colors" },
  { 'C', "no-colors", NULL,  "Disable colors" },
#ifdef HAVE_GETMOUSE
  { 'm', "mouse",    NULL,   "Enable mouse" },
  { 'M', "no-mouse", NULL,   "Disable mouse" },
#endif
  { 'e', "exit",     NULL,   "Exit on connection errors" },
  { 'p', "port",  "PORT", "Connect to server on port [" DEFAULT_PORT_STR "]" },
  { 'h', "host",  "HOST", "Connect to server on host [" DEFAULT_HOST "]" },
  { 'P', "password","PASSWORD", "Connect with password" },
  { 'f', "config",  "FILE",     "Read configuration from file" },
  { 'k', "key-file","FILE",     "Read configuration from file" },
#ifdef DEBUG
  { 'K', "dump-keys", NULL,     "Dump key bindings to stdout" },
  { 'D', "debug",   NULL,   "Enable debug output on stderr" },
#endif
  { 0, NULL, NULL, NULL },
};

static arg_opt_t *
lookup_option(int s, char *l)
{
  int i;

  i=0;
  while( option_table[i].descrition )
    {
      if( l && strcmp(l, option_table[i].longopt) == 0 )
	return &option_table[i];;
      if( s && s==option_table[i].shortopt )
	return &option_table[i];;
      i++;
    }
  return NULL;
}

static void
option_error(int error, char *option, char *arg)
{
  switch(error)
    {
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
  int i = 0;

  printf("Usage: %s [OPTION]...\n", PACKAGE);
  while( option_table[i].descrition )
    {
      char tmp[MAX_LONGOPT_LENGTH];

      if( option_table[i].argument )
	g_snprintf(tmp, MAX_LONGOPT_LENGTH, "%s=%s", 
		   option_table[i].longopt, 
		   option_table[i].argument);
      else
	g_strlcpy(tmp, option_table[i].longopt, 64);

      printf("  -%c, --%-20s %s\n", 
	     option_table[i].shortopt, 
	     tmp,
	     option_table[i].descrition);
      i++;
    }
}

static void 
handle_option(int c, char *arg)
{
  D("option callback -%c %s\n", c, arg);
  switch(c)
    {
    case '?': /* --help */
      display_help();
      exit(EXIT_SUCCESS);
    case 'V': /* --version */
      printf("%s version: %s\n", PACKAGE, VERSION);
      printf("build options:");
#ifdef DEBUG
      printf(" debug");
#endif
#ifdef ENABLE_NLS
      printf(" nls");
#endif
#ifdef ENABLE_KEYDEF_SCREEN
      printf(" key-screen");
#endif
#ifdef ENABLE_CLOCK_SCREEN
      printf(" clock-screen");
#endif
      printf("\n");
      exit(EXIT_SUCCESS);
    case 'c': /* --colors */
      options.enable_colors = TRUE;
      break;
    case 'C': /* --no-colors */
      options.enable_colors = FALSE;
      break;
   case 'm': /* --mouse */
     options.enable_mouse = TRUE;
      break;
    case 'M': /* --no-mouse */
      options.enable_mouse = FALSE;
      break;
    case 'e': /* --exit */
      options.reconnect = FALSE;
      break;
    case 'p': /* --port */
      options.port = atoi(arg);
      break;
    case 'h': /* --host */
      if( options.host )
	g_free(options.host);
      options.host = g_strdup(arg);
      break;
    case 'P': /* --password */
      if( options.password )
	g_free(options.password);
      options.password = locale_to_utf8(arg);
      break;
    case 'f': /* --config */
      if( options.config_file )
	g_free(options.config_file);
      options.config_file = g_strdup(arg);
      break;
    case 'k': /* --key-file */
      if( options.key_file )
	g_free(options.key_file);
      options.key_file = g_strdup(arg);
      break;
#ifdef DEBUG
    case 'K': /* --dump-keys */
      read_configuration(&options);
      write_key_bindings(stdout, KEYDEF_WRITE_ALL | KEYDEF_COMMENT_ALL);
      exit(EXIT_SUCCESS);
      break;
    case 'D': /* --debug */
      options.debug = TRUE;
      break;
#endif
    default:
      fprintf(stderr,"Unknown Option %c = %s\n", c, arg);
      break;
    }
}

options_t *
options_get(void)
{
  return &options;
}

options_t *
options_parse(int argc, const char *argv[])
{
  int i;
  arg_opt_t *opt = NULL;
  option_callback_fn_t option_cb = handle_option;

  i=1;
  while( i<argc )
    {
      char *arg = (char *) argv[i];
      size_t len=strlen(arg);
      
      /* check for a long option */
      if( g_str_has_prefix(arg, "--") )
	{
	  char *name, *value;
	  
	  /* make shure we got an argument for the previous option */
	  if( opt && opt->argument )
	    option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);

	  /* retreive a option argument */
	  if( (value=g_strrstr(arg+2, "=")) )
	    {
	      *value = '\0';
	      name = g_strdup(arg);
	      *value = '=';
	      value++;
	    }
	  else
	    name = g_strdup(arg);

	  /* check if the option exists */
	  if( (opt=lookup_option(0, name+2)) == NULL )
	    option_error(ERROR_UNKNOWN_OPTION, name, NULL);
	  g_free(name);
	  
	  /* abort if we got an argument to the option and dont want one */
	  if( value && opt->argument==NULL )
	    option_error(ERROR_GOT_ARGUMENT, arg, value);
	  
	  /* execute option callback */
	  if( value || opt->argument==NULL )
	    {
	      option_cb (opt->shortopt, value);
	      opt = NULL;
	    }
	}
      /* check for short options */
      else if( len>=2 && g_str_has_prefix(arg, "-") )
	{
	  int j;

	  for(j=1; j<len; j++)
	    {
	      /* make shure we got an argument for the previous option */
	      if( opt && opt->argument )
		option_error(ERROR_MISSING_ARGUMENT, 
			     opt->longopt, opt->argument);
	      
	      /* check if the option exists */
	      if( (opt=lookup_option(arg[j], NULL))==NULL )
		option_error(ERROR_UNKNOWN_OPTION, arg, NULL);

	      /* if no option argument is needed execute callback */
	      if( opt->argument==NULL )
		{
		  option_cb (opt->shortopt, NULL);
		  opt = NULL;
		}
	    }
	}
      else
	{
	  /* is this a option argument? */
	  if( opt && opt->argument)
	    {
	      option_cb (opt->shortopt, arg);
	      opt = NULL;
	    }
	  else 
	    option_error(ERROR_BAD_ARGUMENT, arg, NULL);	  
	}
      i++;
    }
  
  if( opt && opt->argument==NULL)
    option_cb (opt->shortopt, NULL);
  else if( opt && opt->argument )
    option_error(ERROR_MISSING_ARGUMENT, opt->longopt, opt->argument);

  return  &options;
}


options_t *
options_init( void )
{
  const char *value;
  char *tmp;

  memset(&options, 0, sizeof(options_t));

  /* get initial values for host and password from MPD_HOST (enviroment) */
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
  /* get initial values for port from MPD_PORT (enviroment) */
  if( (value=g_getenv(MPD_PORT_ENV)) )
    options.port = atoi(value);
  else
    options.port = DEFAULT_PORT;

  /* default option values */
  options.reconnect = TRUE;
  options.find_wrap = TRUE;
  options.wide_cursor = TRUE;
  options.audible_bell = TRUE;
  options.crossfade_time = DEFAULT_CROSSFADE_TIME;

  return &options;
}

