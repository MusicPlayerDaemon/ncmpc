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
const char *strcasestr(const char *haystack, const char *needle);

typedef struct {
	gsize offset;
	GTime t; /* GTime is equivalent to time_t */
} scroll_state_t;

char *strscroll(char *str, char *separator, int width, scroll_state_t *st);

#endif 
