#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#ifdef HAVE_LOCALE_H
#ifdef HAVE_LANGINFO_H
#ifdef HAVE_ICONV
#include <locale.h>
#include <langinfo.h>
#include <iconv.h>
#define ENABLE_CHARACTER_SET_CONVERSION
#endif
#endif
#endif

#include "support.h"

#define BUFSIZE 1024

#ifdef ENABLE_CHARACTER_SET_CONVERSION
static char *locale = NULL;
static char *charset = NULL;
iconv_t iconv_from_uft8 = (iconv_t)(-1);
iconv_t iconv_to_uft8 = (iconv_t)(-1);
#endif



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


int
charset_init(void)
{
#ifdef ENABLE_CHARACTER_SET_CONVERSION
  /* get current locale */
  if( (locale=setlocale(LC_CTYPE,"")) == NULL )
    {
      fprintf(stderr,"setlocale() - failed!\n");
      return -1;
    }

  /* get charset */
  if( (charset=nl_langinfo(CODESET)) == NULL )
    {
      fprintf(stderr,"nl_langinfo() - failed!\n");
      return -1;
    }

  /* allocate descriptor for character set conversion */
  iconv_from_uft8 = iconv_open(charset, "UTF-8");
  if( iconv_from_uft8 == (iconv_t)(-1) )
    {
      perror("iconv_open");
      return -1;
    }
#endif

  return 0;
}

int
charset_close(void)
{
#ifdef ENABLE_CHARACTER_SET_CONVERSION
  if( iconv_from_uft8 == (iconv_t)(-1) )
    {
      iconv_close(iconv_from_uft8);
      iconv_from_uft8 = (iconv_t)(-1);
    }
  if( iconv_to_uft8 == (iconv_t)(-1) )
    {
      iconv_close(iconv_to_uft8);
      iconv_to_uft8 = (iconv_t)(-1);
    }
#endif
  return 0;
}



char *
utf8_to_locale(char *str)
{
#ifdef ENABLE_CHARACTER_SET_CONVERSION
  size_t inleft;
  size_t retlen;
  char *ret;

  if( iconv_from_uft8 == (iconv_t)(-1) )
    return strdup(str);

  ret = NULL;
  retlen = 0;
  inleft = strlen(str);
  while( inleft>0 )
    {
      char buf[BUFSIZE];
      size_t outleft = BUFSIZE;
      char *bufp = buf;

      if( iconv(iconv_from_uft8, &str, &inleft, &bufp, &outleft) <0 )
	{
	  perror("iconv");
	  free(ret);
	  return NULL;
	}
      ret = realloc(ret, BUFSIZE-outleft+1);
      memcpy(ret+retlen, buf, BUFSIZE-outleft);
      retlen += BUFSIZE-outleft;
      ret[retlen] = '\0';
    }
  return ret;

#else
  return strdup(str);
#endif
}



