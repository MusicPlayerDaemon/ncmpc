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
#include "options.h"
#include "support.h"
#include "command.h"
#include "conf.h"

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define RCFILE "." PACKAGE "rc"

#define MAX_LINE_LENGTH 1024
#define COMMENT_TOKEN   '#'

/* configuration field names */
#define CONF_ENABLE_COLORS           "enable_colors"
#define CONF_AUTO_CENTER             "auto_center"

/* configuration field names - colors */
#define CONF_COLOR_BACKGROUND        "background_color"
#define CONF_COLOR_TITLE             "title_color"
#define CONF_COLOR_LINE              "line_color"
#define CONF_COLOR_LIST              "list_color"
#define CONF_COLOR_PROGRESS          "progress_color"
#define CONF_COLOR_STATUS            "status_color"
#define CONF_COLOR_ALERT             "alert_color"
#define CONF_WIDE_CURSOR             "wide_cursor"
#define CONF_KEY_DEFINITION          "key"

typedef enum {
  KEY_PARSER_UNKNOWN,
  KEY_PARSER_CHAR,
  KEY_PARSER_DEC,
  KEY_PARSER_HEX,
  KEY_PARSER_DONE
} key_parser_state_t;

static int
str2bool(char *str)
{
  if( !strcasecmp(str,"no")  || !strcasecmp(str,"false") || 
      !strcasecmp(str,"off") || !strcasecmp(str,"0") )
    return 0;
  return 1;
}

static int
str2color(char *str)
{
  if( !strcasecmp(str,"black") )
    return COLOR_BLACK;
  else if( !strcasecmp(str,"red") )
    return COLOR_RED;
  else if( !strcasecmp(str,"green") )
    return COLOR_GREEN;
  else if( !strcasecmp(str,"yellow") )
    return COLOR_YELLOW;
  else if( !strcasecmp(str,"blue") )
    return COLOR_BLUE;
  else if( !strcasecmp(str,"magenta") )
    return COLOR_MAGENTA;
  else if( !strcasecmp(str,"cyan") )
    return COLOR_CYAN;
  else if( !strcasecmp(str,"white") )
    return COLOR_WHITE;
  fprintf(stderr,"Warning: unknown color %s\n", str);
  return -1;
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
	    fprintf(stderr, "Error: Unsupported key definition - %s\n", str);
	    return -1;
	  }
	  break;
	case KEY_PARSER_CHAR:
	  if( next!='\'' )
	    {
	      fprintf(stderr, "Error: Unsupported key definition - %s\n", str);
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
	      fprintf(stderr, "Error: Digit expexted after 0x - %s\n", str);
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
      fprintf(stderr, "Error: Unknown key command %s\n", buf);
      return -1;
    }

  /* skip whitespace */
  while( i<len && (str[i]=='=' || IS_WHITESPACE(str[i])) )
    i++;

  /* get the value part */
  memset(buf, 0, MAX_LINE_LENGTH);
  strncpy(buf, str+i, len-i);
  len = strlen(buf);
  if( len==0 )
    {
      fprintf(stderr,"Error: Incomplete key definition - %s\n", str);
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
      fprintf(stderr,"Error: Bad key definition - %s\n", str);
      return -1;
    }

  return assign_keys(cmd, keys);
}

static int
read_rc_file(char *filename, options_t *options)
{
  int fd;
  int quit  = 0;
  int color = -1;
  int free_filename = 0;

  if( filename==NULL )
    return -1;

  D(printf("Reading configuration file %s\n", filename));
  if( (fd=open(filename,O_RDONLY)) <0 )
    {
      D(perror(filename));
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
	      /* background color */
	      else if( !strcasecmp(CONF_COLOR_BACKGROUND, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->bg_color = color;
		}
	      /* color - top (title) window */
	      else if( !strcasecmp(CONF_COLOR_TITLE, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->title_color = color;
		}
	      /* color - line (title) window */
	      else if( !strcasecmp(CONF_COLOR_LINE, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->line_color = color;
		}
	      /* color - list window */
	      else if( !strcasecmp(CONF_COLOR_LIST, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->list_color = color;
		}
	      /* color - progress bar */
	      else if( !strcasecmp(CONF_COLOR_PROGRESS, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->progress_color = color;
		}
	      /* color - status window */
	      else if( !strcasecmp(CONF_COLOR_STATUS, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->status_color = color;
		}
	      /* color - alerts */
	      else if( !strcasecmp(CONF_COLOR_ALERT, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->alert_color = color;
		}
	      else if( !strcasecmp(CONF_WIDE_CURSOR, name) )
		{
		  options->wide_cursor = str2bool(value);
		}
	      else
		{
		  match_found = 0;
		}
	      

	      if( !match_found )
		fprintf(stderr, 
			"Unknown configuration parameter: %s\n", 
			name);
#ifdef DEBUG
	      printf( "  %s = %s %s\n", 
		      name, 
		      value,
		      match_found ? "" : "- UNKNOWN SETTING!" );
#endif

	    }
	}	  
    }

  D(printf( "--\n\n" ));

  if( free_filename )
    g_free(filename);
 
  return 0;
}


int
read_configuration(options_t *options)
{
  char *filename = NULL;

  /* check for user configuration ~/.ncmpc/config */
  filename = g_build_filename(g_get_home_dir(), "." PACKAGE, "config", NULL);
  if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
    {
      g_free(filename);
      filename = NULL;
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

  /* check for  user key bindings ~/.ncmpc/keys */
  filename = g_build_filename(g_get_home_dir(), "." PACKAGE, "keys", NULL);
  if( !g_file_test(filename, G_FILE_TEST_IS_REGULAR) )
    {
      g_free(filename);
      filename = NULL;
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
      //write_key_bindings(stderr);
    }

  return 0;
}



