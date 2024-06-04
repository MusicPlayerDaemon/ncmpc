// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef I18N_H
#define I18N_H

#include "config.h"

#ifdef HAVE_LIBINTL_H
#include <libintl.h> // IWYU pragma: export
#endif

#ifdef ENABLE_NLS

#define _(x) gettext(x)

#ifdef gettext_noop
#define N_(x) gettext_noop(x)
#else
#define N_(x) (x)
#endif

#else
// undef gettext as it can defined in libintl.h even when NLS is disabled by the user
#undef gettext
#define gettext(x) (x)
#define  _(x) x
#define N_(x) x
#endif

#define YES_TRANSLATION _("y")
#define NO_TRANSLATION _("n")

#endif
