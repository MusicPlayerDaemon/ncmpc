/*
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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
 *
 */

#ifndef CHARSET_H
#define CHARSET_H

#include "config.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef ENABLE_LOCALE
const char *
charset_init(void);
#endif

/**
 * Returns the number of terminal cells occupied by this string.
 */
unsigned
utf8_width(const char *str);

char *utf8_to_locale(const char *str);
char *locale_to_utf8(const char *str);

#endif
