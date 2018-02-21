#include "hscroll.hxx"
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

	width = atoi(argv[3]);
	count = atoi(argv[4]);

	hscroll.Init(nullptr, argv[2]);
	hscroll.Set(0, 0, width, argv[1]);

	for (unsigned i = 0; i < count; ++i) {
		p = hscroll.ScrollString();
		g_print("%s\n", p);
		g_free(p);

		hscroll.Step();
	}

	hscroll.Clear();
	return 0;
}
