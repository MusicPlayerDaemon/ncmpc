#ifndef NCMPC_H
#define NCMPC_H

#ifndef PACKAGE
#include "config.h"
#endif

/* i18n */
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifdef ENABLE_NLS
#include <libintl.h>
#include <glib/gi18n.h>
#else
#define  _(x) x
#define N_(x) x
#endif

#define YES   _("y")
#define NO    _("n")

/* welcome message time [s] */
#define SCREEN_WELCOME_TIME 10

/* status message time [s] */
#define SCREEN_STATUS_MESSAGE_TIME 3

/* getch() timeout for non blocking read [ms] */
#define SCREEN_TIMEOUT 500

/* minumum window size */
#define SCREEN_MIN_COLS 14
#define SCREEN_MIN_ROWS  5

/* time between mpd updates [s] */
#define MPD_UPDATE_TIME 0.5

/* time before trying to reconnect [ms] */
#define MPD_RECONNECT_TIME  1500

#define LIST_FORMAT options.list_format

#define STATUS_FORMAT options.status_format

void
sigstop(void);

#endif /* NCMPC_H */
