
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#else
char *basename(char *path);
#endif

char *utf8(char *str);

