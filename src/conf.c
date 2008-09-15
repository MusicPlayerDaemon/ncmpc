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

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>
#include <ncurses.h>
#include "config.h"
#include "ncmpc.h"
#include "options.h"
#include "support.h"
#include "command.h"
#include "colors.h"
#include "conf.h"

#define MAX_LINE_LENGTH 1024
#define COMMENT_TOKEN   '#'

/* configuration field names */
#define CONF_ENABLE_COLORS           "enable-colors"
#define CONF_AUTO_CENTER             "auto-center"
#define CONF_WIDE_CURSOR             "wide-cursor"
#define CONF_ENABLE_BELL             "enable-bell"
#define CONF_KEY_DEFINITION          "key"
#define CONF_COLOR                   "color"
#define CONF_COLOR_DEFINITION        "colordef"
#define CONF_LIST_FORMAT             "list-format"
#define CONF_STATUS_FORMAT           "status-format"
#define CONF_XTERM_TITLE_FORMAT      "xterm-title-format"
#define CONF_LIST_WRAP               "wrap-around"
#define CONF_FIND_WRAP               "find-wrap"
#define CONF_FIND_SHOW_LAST          "find-show-last"
#define CONF_AUDIBLE_BELL            "audible-bell"
#define CONF_VISIBLE_BELL            "visible-bell"
#define CONF_XTERM_TITLE             "set-xterm-title"
#define CONF_ENABLE_MOUSE            "enable-mouse"
#define CONF_CROSSFADE_TIME          "crossfade-time"
#define CONF_SEARCH_MODE             "search-mode"
#define CONF_HIDE_CURSOR             "hide-cursor"
#define CONF_SEEK_TIME               "seek-time"
#define CONF_SCREEN_LIST             "screen-list"
#define CONF_TIMEDISPLAY_TYPE        "timedisplay-type"
#define CONF_HOST                    "host"
#define CONF_PORT                    "port"
#define CONF_PASSWORD                "password"
#define CONF_LYRICS_TIMEOUT          "lyrics-timeout"
#define CONF_SHOW_SPLASH             "show-splash"
#define CONF_SCROLL                  "scroll"
#define CONF_SCROLL_SEP              "scroll-sep"

typedef enum {
  KEY_PARSER_UNKNOWN,
  KEY_PARSER_CHAR,
  KEY_PARSER_DEC,
  KEY_PARSER_HEX,
  KEY_PARSER_DONE
} key_parser_state_t;


extern gint screen_get_id(char *name);


static gboolean
str2bool(char *str)
{
  if( !strcasecmp(str,"yes")  || !strcasecmp(str,"true") || 
      !strcasecmp(str,"on")   || !strcasecmp(str,"1") )
    return TRUE;
  return FALSE;
}

static int
parse_key_value(char *str, size_t len, char **end)
{
  int i, value;
  key_parser_state_t state;

  i=0;
  value=0;
  state=KEY_PARSER_UNKNOWN;
  *end = str;

  while( i<len && state!=KEY_PARSER_DONE )
    {
      int next = 0;
      int c = str[i];

      if( i+1<len )
	next = str[i+1];

      switch(state)
	{
	case KEY_PARSER_UNKNOWN:
	  if( c=='\'' )
	    state = KEY_PARSER_CHAR;
	  else if( c=='0' && next=='x' )
	    state = KEY_PARSER_HEX;
	  else if( isdigit(c) )
	    state = KEY_PARSER_DEC;
	  else {
	    fprintf(stderr,
		    _("Error: Unsupported key definition - %s\n"), 
		    str);
	    return -1;
	  }
	  break;
	case KEY_PARSER_CHAR:
	  if( next!='\'' )
	    {
	      fprintf(stderr,
		      _("Error: Unsupported key definition - %s\n"), 
		      str);
	      return -1;
	    }
	  value = c;
	  *end = str+i+2;
	  state = KEY_PARSER_DONE;
	  break;
	case KEY_PARSER_DEC:
	  value = (int) strtol(str+(i-1), end, 10);
	  state = KEY_PARSER_DONE;
	  break;
	case KEY_PARSER_HEX:
	  if( !isdigit(next) )
	    {
	      fprintf(stderr,_("Error: Digit expected after 0x - %s\n"), str);
	      return -1;
	    }
	  value = (int) strtol(str+(i+1), end, 16);
	  state = KEY_PARSER_DONE;
	  break;
	case KEY_PARSER_DONE:
	  break;
	}
      i++;
    }

  if( *end> str+len )
    *end = str+len;

  return value;
}

static int
parse_key_definition(char *str)
{
  char buf[MAX_LINE_LENGTH];
  char *p, *end;
  size_t len = strlen(str);
  int i,j,key;
  int keys[MAX_COMMAND_KEYS];
  command_t cmd;

  /* get the command name */
  i=0;
  j=0;
  memset(buf, 0, MAX_LINE_LENGTH);
  while( i<len && str[i]!='=' && !IS_WHITESPACE(str[i]) )
    buf[j++] = str[i++];
  if( (cmd=get_key_command_from_name(buf)) == CMD_NONE )
    {
      fprintf(stderr, _("Error: Unknown key command %s\n"), buf);
      return -1;
    }

  /* skip whitespace */
  while( i<len && (str[i]=='=' || IS_WHITESPACE(str[i])) )
    i++;

  /* get the value part */
  memset(buf, 0, MAX_LINE_LENGTH);
  g_strlcpy(buf, str+i, MAX_LINE_LENGTH);
  len = strlen(buf);
  if( len==0 )
    {
      fprintf(stderr,_("Error: Incomplete key definition - %s\n"), str);
      return -1;
    }

  /* parse key values */
  i = 0;
  key = 0;
  len = strlen(buf);
  p = buf;
  end = buf+len;
  memset(keys, 0, sizeof(int)*MAX_COMMAND_KEYS);
  while( i<MAX_COMMAND_KEYS && p<end && (key=parse_key_value(p,len+1,&p))>=0 )
    {
      keys[i++] = key;
      while( p<end && (*p==',' || *p==' ' || *p=='\t') )
	p++;
      len = strlen(p);
    } 
  if( key<0 )
    {
      fprintf(stderr,_("Error: Bad key definition - %s\n"), str);
      return -1;
    }

  return assign_keys(cmd, keys);
}

static char *
parse_timedisplay_type(char *str)
{
 if((!strcmp(str,"elapsed")) || (!strcmp(str,"remaining"))){
   return str;
 } else {
   fprintf(stderr,_("Error: Bad time display type - %s\n"), str);
   return DEFAULT_TIMEDISPLAY_TYPE;
 }
}

static int
parse_color(char *str)
{
  char *name = str;
  char *value = NULL;
  int len,i;

  i=0;
  len=strlen(str);
  /* get the color name */
  while( i<len && str[i]!='=' && !IS_WHITESPACE(str[i]) )
    i++;
  
  /* skip whitespace */
  while( i<len && (str[i]=='=' || IS_WHITESPACE(str[i])) )
    {
      str[i]='\0';
      i++;
    } 
  
  if( i<len )
    value = str+i;

  return colors_assign(name, value);
}

static int
parse_color_definition(char *str)
{
  char buf[MAX_LINE_LENGTH];
  char *p, *end, *name;
  size_t len = strlen(str);
  int i,j,value;
  short color, rgb[3];

  /* get the command name */
  i=0;
  j=0;
  memset(buf, 0, MAX_LINE_LENGTH);
  while( i<len && str[i]!='=' && !IS_WHITESPACE(str[i]) )
    buf[j++] = str[i++];
  color=colors_str2color(buf);
  if( color<0 )
    {
      fprintf(stderr,_("Error: Bad color %s [%d]\n"), buf, color);
      return -1;
    }
  name = g_strdup(buf);

  /* skip whitespace */
  while( i<len && (str[i]=='=' || IS_WHITESPACE(str[i])) )
    i++;

  /* get the value part */
  memset(buf, 0, MAX_LINE_LENGTH);
  g_strlcpy(buf, str+i, MAX_LINE_LENGTH);
  len = strlen(buf);
  if( len==0 )
    {
      fprintf(stderr,_("Error: Incomplete color definition - %s\n"), str);
      g_free(name);
      return -1;
    }

  /* parse r,g.b values with the key definition parser */
  i = 0;
  value = 0;
  len = strlen(buf);
  p = buf;
  end = buf+len;
  memset(rgb, 0, sizeof(short)*3);
  while( i<3 && p<end && (value=parse_key_value(p,len+1,&p))>=0 )
    {
      rgb[i++] = value;
      while( p<end && (*p==',' || *p==' ' || *p=='\t') )
	p++;
      len = strlen(p);
    } 
  if( value<0 || i!=3)
    {
      fprintf(stderr,_("Error: Bad color definition - %s\n"), str);
      g_free(name);
      return -1;
    }
  value = colors_define(name, rgb[0], rgb[1], rgb[2]);
  g_free(name);
  return value;
}

static char *
get_format(char *str)
{
  gsize len = strlen(str);

  if( str && str[0]=='\"' && str[len-1] == '\"' )
    {
      str[len-1] = '\0';
      str++;
    }
  return g_strdup(str);
}

static char **
check_screen_list(char *value)
{
  char **tmp = g_strsplit_set(value, " \t,", 100);
  char **screen = NULL;
  int i,j;

  i=0;
  j=0;
  while( tmp && tmp[i] )
    {
      tmp[i] = lowerstr(tmp[i]);
      if( screen_get_id(tmp[i]) == -1 )
	fprintf(stderr,
		_("Error: Unsupported screen \"%s\"\n"), 
		tmp[i]);
      else
	{
	  screen = g_realloc(screen, (j+2)*sizeof(char *));
	  screen[j++] = g_strdup(tmp[i]);
	  screen[j] = NULL;
	}
      i++;
    }
  g_strfreev(tmp);
  if( screen == NULL )
    return g_strsplit_set(DEFAULT_SCREEN_LIST, " ", 0);

  return screen;
}

static int
read_rc_file(char *filename, options_t *options)
{
  int fd;
  int quit  = 0;
  int free_filename = 0;

  if( filename==NULL )
    return -1;

  D("Reading configuration file %s\n", filename);
  if( (fd=open(filename,O_RDONLY)) <0 )
    {
      perror(filename);
      if( free_filename )
	g_free(filename);
      return -1;
    }

  while( !quit )
    {
      int i,j;
      int len;
      int match_found;
      char line[MAX_LINE_LENGTH];
      char name[MAX_LINE_LENGTH];
      char value[MAX_LINE_LENGTH];

      line[0]  = '\0';
      value[0] = '\0';

      i = 0;
      /* read a line ending with '\n' */
      do {
	len = read( fd, &line[i], 1 );
	if( len == 1 )
	  i++;
	else
	  quit = 1;
      } while( !quit && i<MAX_LINE_LENGTH && line[i-1]!='\n' );
      
     
      /* remove trailing whitespace */
      line[i] = '\0';
      i--;
      while( i>=0 && IS_WHITESPACE(line[i]) )
	{
	  line[i] = '\0';
	  i--;
	}     
      len = i+1;

      if( len>0 )
	{
	  i = 0;
	  /* skip whitespace */
	  while( i<len && IS_WHITESPACE(line[i]) )
	    i++;
	  
	  /* continue if this line is not a comment */
	  if( line[i] != COMMENT_TOKEN )
	    {
	      /* get the name part */
	      j=0;
	      while( i<len && line[i]!='=' && !IS_WHITESPACE(line[i]) )
		{
		  name[j++] = line[i++];
		}
	      name[j] = '\0';
	      
	      /* skip '=' and whitespace */
	      while( i<len && (line[i]=='=' || IS_WHITESPACE(line[i])) )
		i++;
	      
	      /* get the value part */
	      j=0;
	      while( i<len )
		{
		  value[j++] = line[i++];
		}
	      value[j] = '\0';
	      
	      match_found = 1;

	      /* key definition */
	      if( !strcasecmp(CONF_KEY_DEFINITION, name) )
		{
		  parse_key_definition(value);
		}
	      /* enable colors */
	      else if( !strcasecmp(CONF_ENABLE_COLORS, name) )
		{
		  options->enable_colors = str2bool(value);
		}
	      /* auto center */
	      else if( !strcasecmp(CONF_AUTO_CENTER, name) )
		{
		  options->auto_center = str2bool(value);
		}
	      /* color assignment */
	      else if( !strcasecmp(CONF_COLOR, name) )
		{
		  parse_color(value);
		}
	      /* wide cursor */
	      else if( !strcasecmp(CONF_WIDE_CURSOR, name) )
		{
		  options->wide_cursor = str2bool(value);
		}
	      /* timer display type */
	      else if( !strcasecmp(CONF_TIMEDISPLAY_TYPE, name) )
		{
		    g_free(options->timedisplay_type);
		    options->timedisplay_type=g_strdup(parse_timedisplay_type(value));
		    D("deb");
		    D(options->timedisplay_type);
		}
	      /* color definition */
	      else if( !strcasecmp(CONF_COLOR_DEFINITION, name) )
		{
		  parse_color_definition(value);
		}
	      /* list format string */
	      else if( !strcasecmp(CONF_LIST_FORMAT, name) )
		{
		  g_free(options->list_format);
		  options->list_format = get_format(value);
		}
	      /* status format string */
	      else if( !strcasecmp(CONF_STATUS_FORMAT, name) )
		{
		  g_free(options->status_format);
		  options->status_format = get_format(value);
		}
	      /* xterm title format string */
	      else if( !strcasecmp(CONF_XTERM_TITLE_FORMAT, name) )
		{
		  g_free(options->xterm_title_format);
		  options->xterm_title_format = get_format(value);
		}
	      else if( !strcasecmp(CONF_LIST_WRAP, name) )
		{
		  options->list_wrap = str2bool(value);
		}
	      else if( !strcasecmp(CONF_FIND_WRAP, name) )
		{
		  options->find_wrap = str2bool(value);
		}
	      else if( !strcasecmp(CONF_FIND_SHOW_LAST,name) )
		{
		  options->find_show_last_pattern = str2bool(value);
		}
	      else if( !strcasecmp(CONF_AUDIBLE_BELL, name) )
		{
		  options->audible_bell = str2bool(value);
		}
	      else if( !strcasecmp(CONF_VISIBLE_BELL, name) )
		{
		  options->visible_bell = str2bool(value);
		}
	      else if( !strcasecmp(CONF_XTERM_TITLE, name) )
		{
		  options->enable_xterm_title = str2bool(value);
		}
	      else if( !strcasecmp(CONF_ENABLE_MOUSE, name) )
		{
		  options->enable_mouse = str2bool(value);
		}
	      else if( !strcasecmp(CONF_CROSSFADE_TIME, name) )
		{
		  options->crossfade_time = atoi(value);
		}
	      else if( !strcasecmp(CONF_SEARCH_MODE, name) )
		{
		  options->search_mode = atoi(value);
		}
	      else if( !strcasecmp(CONF_HIDE_CURSOR, name) )
		{
		  options->hide_cursor = atoi(value);
		}
	      else if( !strcasecmp(CONF_SEEK_TIME, name) )
		{
		  options->seek_time = atoi(value);
		}
	      else if( !strcasecmp(CONF_SCREEN_LIST, name) )
		{
		  g_strfreev(options->screen_list);
		  options->screen_list = check_screen_list(value);
		}
	      else if( !strcasecmp(CONF_SHOW_SPLASH, name) )
		{
		  /* the splash screen was removed */
		}
	    else if( !strcasecmp(CONF_HOST, name))
	    {
	    options->host = get_format(value);
	    }
	    else if( !strcasecmp(CONF_PORT, name))
	    {
	    options->port = atoi(get_format(value));
	    }
	    else if( !strcasecmp(CONF_PASSWORD, name))
	      {
		options->password = get_format(value);
	      }
	    else if( !strcasecmp(CONF_LYRICS_TIMEOUT, name))
	    {
	    options->lyrics_timeout = atoi(get_format(value));
	    }		
	    else if( !strcasecmp(CONF_SCROLL, name))
	    {
	    options->scroll = str2bool(value);
	    }
	    else if( !strcasecmp(CONF_SCROLL_SEP, name))
	    {
	    g_free(options->scroll_sep);
	    options->scroll_sep = get_format(value);
	    }
	      else
		{
		  match_found = 0;
		}

	      if( !match_found )
		fprintf(stderr, 
			_("Unknown configuration parameter: %s\n"), 
			name);
	      D("conf>  %s = %s %s\n", name, value,
		match_found ? "" : "- UNKNOWN SETTING!" );
	    }
	}	  
    }

  D("--\n\n");

  if( free_filename )
    g_free(filename);
 
  return 0;
}

int
check_user_conf_dir(void)
{
  int retval;
  char *dirname = g_build_filename(g_get_home_dir(), "." PACKAGE, NULL);
  
  if( g_file_test(dirname, G_FILE_TEST_IS_DIR) )
    {
      g_free(dirname);
      return 0;
    }
  retval = mkdir(dirname, 0755);
  g_free(dirname);
  return retval;
}

char *
get_user_key_binding_filename(void)
{
  return g_build_filename(g_get_home_dir(), "." PACKAGE, "keys", NULL);
}


int
read_configuration(options_t *options)
{
  char *filename = NULL;

  /* check for command line configuration file */
  if( options->config_file )
    filename = g_strdup(options->config_file);

  /* check for user configuration ~/.ncmpc/config */
  if( filename == NULL )
    {
      filename = g_build_filename(g_get_home_dir(), 
				  "." PACKAGE, "config", NULL);
      if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
	{
	  g_free(filename);
	  filename = NULL;
	}
    }

  /* check for  global configuration SYSCONFDIR/ncmpc/config */
  if( filename == NULL )
    {
      filename = g_build_filename(SYSCONFDIR, PACKAGE, "config", NULL);
      if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
	{
	  g_free(filename);
	  filename = NULL;
	}
    }

  /* load configuration */
  if( filename )
    {
      read_rc_file(filename, options);
      g_free(filename);
      filename = NULL;
    }

  /* check for command line key binding file */
  if( options->key_file )
    filename = g_strdup(options->key_file);

  /* check for  user key bindings ~/.ncmpc/keys */
  if( filename == NULL )
    {
      filename = get_user_key_binding_filename();
      if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
	{
	  g_free(filename);
	  filename = NULL;
	}
    }

  /* check for  global key bindings SYSCONFDIR/ncmpc/keys */
  if( filename == NULL )
    {
      filename = g_build_filename(SYSCONFDIR, PACKAGE, "keys", NULL);
      if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
	{
	  g_free(filename);
	  filename = NULL;
	}
    }

  /* load key bindings */
  if( filename )
    {
      read_rc_file(filename, options);
      g_free(filename);
      filename = NULL;
    }

  return 0;
}
