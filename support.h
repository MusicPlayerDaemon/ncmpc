
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifndef HAVE_LANGINFO_CODESET
#define CODESET ((nl_item) 1)
typedef int nl_item;
char *nl_langinfo(nl_item);
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


