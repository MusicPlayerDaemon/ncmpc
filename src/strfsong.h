#ifndef STRFSONG_H
#define STRFSONG_H

#include <glib.h>
#include "libmpdclient.h"

gsize strfsong(gchar *s, gsize max, const gchar *format, mpd_Song *song);

#endif
