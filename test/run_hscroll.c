#include "hscroll.h"
#include "config.h"

#include <glib.h>
#include <stdlib.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

int main(int argc, char **argv)
{
	struct hscroll hscroll;
	char *p;
	unsigned width, count;

	if (argc != 5) {
		g_printerr("Usage: %s TEXT SEPARATOR WIDTH COUNT\n", argv[0]);
		return 1;
	}

#ifdef ENABLE_LOCALE
	setlocale(LC_CTYPE,"");
#endif

	hscroll_reset(&hscroll);

	width = atoi(argv[3]);
	count = atoi(argv[4]);

	for (unsigned i = 0; i < count; ++i) {
		p = strscroll(&hscroll, argv[1], argv[2], width);
		g_print("%s\n", p);
		g_free(p);

		hscroll_step(&hscroll);
	}

	hscroll_reset(&hscroll);

	return 0;
}
