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
#include <glib.h>

#include "config.h"
#include "ncmpc.h"
#include "support.h"
#include "options.h"

#define MAX_LONGOPT_LENGTH 32

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
  { 'e', "exit",     NULL,   "Exit on connection errors" },
  { 'p', "port",  "PORT", "Connect to server on port [" DEFAULT_PORT_STR "]" },
  { 'h', "host",  "HOST", "Connect to server on host [" DEFAULT_HOST "]" },
  { 'P', "password","PASSWORD", "Connect with password" },
  { 'f', "config",  "FILE",     "Read configuration from file" },
  { 'k', "key-file","FILE",     "Read configuration from file" },
#ifdef DEBUG
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
  switch(c)
    {
    case '?': /* --help */
      display_help();
      exit(EXIT_SUCCESS);
    case 'V': /* --version */
      printf("Version %s\n", VERSION);
      exit(EXIT_SUCCESS);
    case 'c': /* --colors */
      options.enable_colors = TRUE;
      break;
    case 'C': /* --no-colors */
      options.enable_colors = FALSE;
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
    case 'D': /* --debug */
      options.debug = TRUE;
      break;
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
	  char *value;

	  /* retreive a option argument */
	  if( (value=g_strrstr(arg+2, "=")) )
	    {
	      *value = '\0';
	      value++;
	    }
	  /* check if the option exists */
	  if( (opt=lookup_option(0, arg+2)) == NULL )
	    {
	      fprintf(stderr, PACKAGE ": invalid option %s\n", arg);
	      exit(EXIT_FAILURE);
	    }
	  /* abort if we got an argument to the option and dont want one */
	  if( value && opt->argument==NULL )
	    {
	      fprintf(stderr, PACKAGE ": invalid option argument %s=%s\n", 
		      arg, value);
	      exit(EXIT_FAILURE);
	    }
	  /* execute option callback */
	  if( value || opt->argument==NULL )
	    {
	      option_cb (opt->shortopt, value);
	      opt = NULL;
	    }
	}
      /* check for a short option */
      else if( len==2 && g_str_has_prefix(arg, "-") )
	{
	  /* check if the option exists */
	  if( (opt=lookup_option(arg[1], NULL))==NULL )
	    {
	      fprintf(stderr, PACKAGE ": invalid option %s\n",arg);
	      exit(EXIT_FAILURE);
	    }
	  /* if no option argument is needed execute callback */
	  if( opt->argument==NULL )
	    {
	      option_cb (opt->shortopt, NULL);
	      opt = NULL;
	    }
	}
      else
	{
	  /* is this a option argument? */
	  if( opt && opt->argument)
	    {
	      option_cb (opt->shortopt, arg);
	    }
	  else 
	    {
	      fprintf(stderr, PACKAGE ": bad argument: %s\n", arg);
	      exit(EXIT_FAILURE);
	    }
	  opt = NULL;
	}

      i++;
    }
  return  &options;
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

  options.list_format = NULL;
  options.status_format = NULL;
  options.xterm_title_format = NULL;

  options.reconnect = TRUE;
  options.find_wrap = TRUE;
  options.wide_cursor = TRUE;
  options.audible_bell = TRUE;

  return &options;
}

