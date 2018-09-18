#include "BasicMarquee.hxx"
#include "config.h"

#include <stdlib.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

#include <stdio.h>

int main(int argc, char **argv)
{
	unsigned width, count;

	if (argc != 5) {
		fprintf(stderr, "Usage: %s TEXT SEPARATOR WIDTH COUNT\n", argv[0]);
		return 1;
	}

#ifdef ENABLE_LOCALE
	setlocale(LC_CTYPE,"");
#endif

	width = atoi(argv[3]);
	count = atoi(argv[4]);

	BasicMarquee hscroll(argv[2]);
	hscroll.Set(width, argv[1]);

	for (unsigned i = 0; i < count; ++i) {
		const auto s = hscroll.ScrollString();
		fprintf(stderr, "%.*s\n", int(s.second), s.first);

		hscroll.Step();
	}

	hscroll.Clear();
	return 0;
}
