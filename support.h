
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#else
char *basename(char *path);
#endif



int charset_init(void);
int charset_close(void);
char *utf8_to_locale(char *str);


