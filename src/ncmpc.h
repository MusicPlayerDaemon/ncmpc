#ifndef NCMPC_H
#define NCMPC_H

#ifndef PACKAGE
#include "config.h"
#endif

#ifdef DEBUG
#define D(x, args...) fprintf(stderr, x,  ##args)
#else
#define D(x,...)
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

/* getch() timeout for non blocking read [ms] */
#define SCREEN_TIMEOUT 500

/* time in seconds between mpd updates (double) */
#define MPD_UPDATE_TIME        0.5

/* time in milliseconds before trying to reconnect (int) */
#define MPD_RECONNECT_TIME  1500

/* song format - list window */
#define DEFAULT_LIST_FORMAT "%name%|[%artist% - ]%title%|%file%"
#define LIST_FORMAT (options.list_format ? options.list_format : DEFAULT_LIST_FORMAT)

/* song format - status window */
#define DEFAULT_STATUS_FORMAT "[%artist% - ]%title%|%basename%"
#define STATUS_FORMAT (options.status_format ? options.status_format : DEFAULT_STATUS_FORMAT)

#endif /* NCMPC_H */
