/*
 * $Id: support.c,v 1.2 2004/03/17 23:17:09 kalle Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "support.h"

#ifndef HAVE_LIBGEN_H

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

#endif /* HAVE_LIBGEN_H */

char *
utf8(char *str)
{
  static const gchar *charset = NULL;
  static gboolean locale_is_utf8 = FALSE;

  if( !charset )
    locale_is_utf8 = g_get_charset(&charset);

  if( locale_is_utf8 )
    return str;

  return g_locale_from_utf8(str, -1, NULL, NULL, NULL);
}
