#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "support.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#define BUFSIZE 1024

static const char *charset = NULL;
static const char *locale = NULL;
static gboolean noconvert = TRUE;

char *
trim(char *str)
{
  char *end;

  if( str==NULL )
    return NULL;

  while( IS_WHITESPACE(*str) )
    str++;

  end=str+strlen(str)-1;
  while( end>str && IS_WHITESPACE(*end) )
    {
      *end = '\0';
      end--;
    }
  return str;
}

char *
remove_trailing_slash(char *path)
{
  int len;

  if( path==NULL )
    return NULL;

  len=strlen(path);
  if( len>1 && path[len-1] == '/' )
    path[len-1] = '\0';

  return path;
}

char *
lowerstr(char *str)
{
  size_t i;
  size_t len = strlen(str);

  if( str==NULL )
    return NULL;

  i=0;
  while(i<len && str[i])
    {
      str[i] = tolower(str[i]);
      i++;
    }
  return str;
}


#ifndef HAVE_BASENAME
char *
basename(char *path)
{
  char *end;

  path = remove_trailing_slash(path);
  end = path + strlen(path);

  while( end>path && *end!='/' )
    end--;

  if( *end=='/' && end!=path )
    return end+1;

  return path;
}
#endif /* HAVE_BASENAME */


#ifndef HAVE_STRCASESTR
char *
strcasestr(const char *haystack, const char *needle)
{
  return strstr(lowerstr(haystack), lowerstr(needle));
}
#endif /* HAVE_STRCASESTR */


int
charset_init(void)
{
#ifdef HAVE_LOCALE_H
  /* get current locale */
  if( (locale=setlocale(LC_CTYPE,"")) == NULL )
    {
      g_printerr("setlocale() - failed!\n");
      return -1;
    }
#endif

  /* get charset */
  noconvert = g_get_charset(&charset);

#ifdef DEBUG
  g_printerr("charset: %s\n", charset);
#endif
  
  return 0;
}

int
charset_close(void)
{
  return 0;
}

char *
utf8_to_locale(char *utf8str)
{
  char *str;

  if( noconvert )
    return g_strdup(utf8str);
  if( (str=g_locale_from_utf8(utf8str, -1, NULL, NULL, NULL)) == NULL )
    return g_strdup(utf8str);
  return str;
}

char *
locale_to_utf8(char *localestr)
{
  char *str;

  if( noconvert )
    return g_strdup(localestr);
  if( (str=g_locale_to_utf8(localestr, -1, NULL, NULL, NULL)) == NULL )
    return g_strdup(localestr);
  return str;
}



