#ifndef NCMPC_H
#define NCMPC_H

#ifndef PACKAGE
#include "config.h"
#endif

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

/* i18n */
#ifdef ENABLE_NLS
#include <locale.h>
#include <libintl.h>
#include <glib/gi18n.h>
#else
#define  _(x) x
#define N_(x) x
#endif

/* welcome message time [s] */
#define SCREEN_WELCOME_TIME 10

/* getch() timeout for non blocking read [ms] */
#define SCREEN_TIMEOUT 500

/* time in seconds between mpd updates (double) */
#define MPD_UPDATE_TIME        0.5

/* timout in seconds before trying to reconnect (int) */
#define MPD_RECONNECT_TIMEOUT  3


#endif /* NCMPC_H */
