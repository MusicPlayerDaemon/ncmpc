#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "support.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#define BUFSIZE 1024


static char *charset = NULL;

#ifdef HAVE_LOCALE_H
static char *locale = NULL;
#endif

#ifdef HAVE_ICONV
iconv_t iconv_from_uft8 = (iconv_t)(-1);
iconv_t iconv_to_uft8 = (iconv_t)(-1);
#endif


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


char *
concat_path(char *p1, char *p2)
{
  size_t size;
  char *path;
  char append_slash = 0;

  size = strlen(p1);
  if( size==0 || p1[size-1]!='/' )
    {
      size++;
      append_slash = 1;
    }
  size += strlen(p2);
  size++;

  path = calloc(size, sizeof(char));
  strncpy(path, p1, size);
  if( append_slash )
    strncat(path, "/", size);
  strncat(path, p2, size);

  return path;
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
      fprintf(stderr,"setlocale() - failed!\n");
      return -1;
    }
#endif

  /* get charset */
  if( (charset=nl_langinfo(CODESET)) == NULL )
    {
      fprintf(stderr,
	      "nl_langinfo() failed using default:" DEFAULT_CHARSET "\n");
      charset = DEFAULT_CHARSET;
    }
#ifdef DEBUG
  fprintf(stderr, "charset: %s\n", charset);
#endif
  
#ifdef HAVE_ICONV
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
#ifdef HAVE_ICONV
  if( iconv_from_uft8 != (iconv_t)(-1) )
    {
      iconv_close(iconv_from_uft8);
      iconv_from_uft8 = (iconv_t)(-1);
    }
  if( iconv_to_uft8 != (iconv_t)(-1) )
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
#ifdef HAVE_ICONV
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



