
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

char *concat_path(char *p1, char *p2);

#ifndef HAVE_BASENAME
char *basename(char *path);
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(const char *haystack, const char *needle);
#endif

char *remove_trailing_slash(char *path);
char *lowerstr(char *str);


int charset_init(void);
int charset_close(void);
char *utf8_to_locale(char *str);


