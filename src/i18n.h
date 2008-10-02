/* ncmpc
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef I18N_H
#define I18N_H

#include "config.h"

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

#define YES _("y")
#define NO _("n")

#endif
