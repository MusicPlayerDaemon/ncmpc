#ifndef SUPPORT_H
#define SUPPORT_H

#include <glib.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifndef HAVE_BASENAME
char *basename(char *path);
#endif

#define IS_WHITESPACE(c) (c==' ' || c=='\t' || c=='\r' || c=='\n')

char *remove_trailing_slash(char *path);
char *lowerstr(char *str);
char *strcasestr(const char *haystack, const char *needle);

typedef struct {
	int offset;
	GTime t; /* GTime is equivalent to time_t */
} scroll_state_t;

char *strscroll(char *str, char *separator, int width, scroll_state_t *st);

void charset_init(gboolean disable);
char *utf8_to_locale(const char *str);
char *locale_to_utf8(const char *str);

/* number of characters in str */
size_t my_strlen(const char *str);
/* number of bytes in str */
size_t my_strsize(char *str);


#endif 
