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
#define CONF_COLOR_BACKGROUND        "background_color"
#define CONF_COLOR_TITLE             "title_color"
#define CONF_COLOR_LINE              "line_color"
#define CONF_COLOR_LIST              "list_color"
#define CONF_COLOR_PROGRESS          "progress_color"
#define CONF_COLOR_STATUS            "status_color"
#define CONF_COLOR_ALERT             "alert_color"


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
#if 0
   else if( !strcasecmp(str,"grey") ) 
     return COLOR_BLACK | A_BOLD;
  else if( !strcasecmp(str,"brightred") )
    return COLOR_RED | A_BOLD;
  else if( !strcasecmp(str,"brightgreen") )
    return COLOR_GREEN | A_BOLD;
  else if( !strcasecmp(str,"brightyellow") )
    return COLOR_YELLOW | A_BOLD;
  else if( !strcasecmp(str,"brightblue") )
    return COLOR_BLUE | A_BOLD;
  else if( !strcasecmp(str,"brightmagenta") )
    return COLOR_MAGENTA | A_BOLD;
  else if( !strcasecmp(str,"brightcyan") )
    return COLOR_CYAN | A_BOLD;
  else if( !strcasecmp(str,"brightwhite") )
    return COLOR_WHITE | A_BOLD;
#endif
  fprintf(stderr,"Warning: unknown color %s\n", str);
  return -1;
}

int
read_rc_file(char *filename, options_t *options)
{
  int fd;
  int quit  = 0;
  int color = -1;
  int free_filename = 0;

  if( filename==NULL )
    {
      filename = concat_path(getenv("HOME"), RCFILE);
      free_filename = 1;
    }

  D(printf("\n--Reading configuration file %s\n", filename));
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
	      
	      match_found = 0;
	      
	      /* enable colors */
	      if( !strcasecmp(CONF_ENABLE_COLORS, name) )
		{
		  options->enable_colors = str2bool(value);
		  match_found = 1;
		}
	      /* background color */
	      else if( !strcasecmp(CONF_COLOR_BACKGROUND, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->bg_color = color;
		  match_found = 1;
		}
	      /* color - top (title) window */
	      else if( !strcasecmp(CONF_COLOR_TITLE, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->title_color = color;
		  match_found = 1;
		}
	      /* color - line (title) window */
	      else if( !strcasecmp(CONF_COLOR_LINE, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->line_color = color;
		  match_found = 1;
		}
	      /* color - list window */
	      else if( !strcasecmp(CONF_COLOR_LIST, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->list_color = color;
		  match_found = 1;
		}
	      /* color - progress bar */
	      else if( !strcasecmp(CONF_COLOR_PROGRESS, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->progress_color = color;
		  match_found = 1;
		}
	      /* color - status window */
	      else if( !strcasecmp(CONF_COLOR_STATUS, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->status_color = color;
		  match_found = 1;
		}
	      /* color - alerts */
	      else if( !strcasecmp(CONF_COLOR_ALERT, name) )
		{
		  if( (color=str2color(value)) >= 0 )
		    options->alert_color = color;
		  match_found = 1;
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






