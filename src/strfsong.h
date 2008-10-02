#ifndef STRFSONG_H
#define STRFSONG_H

#include <glib.h>
#include "libmpdclient.h"

gsize strfsong(gchar *s, gsize max, const gchar *format,
	       const struct mpd_song *song);

#endif
