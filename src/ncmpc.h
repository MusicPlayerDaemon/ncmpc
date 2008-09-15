#ifndef NCMPC_H
#define NCMPC_H

#ifndef PACKAGE
#include "config.h"
#endif

#ifndef DISABLE_ARTIST_SCREEN
#define ENABLE_ARTIST_SCREEN 1
#endif
#ifndef DISABLE_SEARCH_SCREEN
#define ENABLE_SEARCH_SCREEN 1
#endif
#ifndef DISABLE_KEYDEF_SCREEN
#define ENABLE_KEYDEF_SCREEN 1
#endif
#ifndef DISABLE_CLOCK_SCREEN
#define ENABLE_CLOCK_SCREEN 1
#endif
#ifndef DISABLE_LYRICS_SCREEN
#define ENABLE_LYRICS_SCREEN 1
#endif

#ifndef NDEBUG
void D(const char *format, ...);
#else
#define D(...)
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

/* mpd crossfade time [s] */
#define DEFAULT_CROSSFADE_TIME 10

/* welcome message time [s] */
#define SCREEN_WELCOME_TIME 10

/* screen list */
#define DEFAULT_SCREEN_LIST "playlist browse"

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

/* song format - list window */
#define DEFAULT_LIST_FORMAT "%name%|[%artist% - ]%title%|%shortfile%"
#define LIST_FORMAT (options.list_format ? options.list_format : \
                                           DEFAULT_LIST_FORMAT)

/* song format - status window */
#define DEFAULT_STATUS_FORMAT "[%artist% - ]%title%|%shortfile%"
#define STATUS_FORMAT (options.status_format ? options.status_format : \
                                               DEFAULT_STATUS_FORMAT)
											   
#define DEFAULT_LYRICS_TIMEOUT 100

#define DEFAULT_SCROLL TRUE
#define DEFAULT_SCROLL_SEP " *** "

void
sigstop(void);

#endif /* NCMPC_H */
