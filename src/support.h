#ifndef SUPPORT_H
#define SUPPORT_H

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifndef HAVE_BASENAME
char *basename(char *path);
#endif

#define IS_WHITESPACE(c) (c==' ' || c=='\t' || c=='\r' || c=='\n')

char *trim(char *str);
char *remove_trailing_slash(char *path);
char *lowerstr(char *str);
char *strcasestr(const char *haystack, const char *needle);

int charset_init(void);
int charset_close(void);
char *utf8_to_locale(char *str);
char *locale_to_utf8(char *str);

#endif 
