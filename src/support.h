#ifndef SUPPORT_H
#define SUPPORT_H

#include <glib.h>

const char *strcasestr(const char *haystack, const char *needle);

typedef struct {
	gsize offset;
	GTime t; /* GTime is equivalent to time_t */
} scroll_state_t;

char *strscroll(char *str, char *separator, int width, scroll_state_t *st);

#endif
