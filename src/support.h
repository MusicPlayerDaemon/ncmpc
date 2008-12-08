/*
 * (c) 2004-2008 The Music Player Daemon Project
 * http://www.musicpd.org/
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

#ifndef SUPPORT_H
#define SUPPORT_H

#include "config.h"

#include <glib.h>

#ifndef HAVE_STRCASESTR
const char *strcasestr(const char *haystack, const char *needle);
#endif

#ifndef NCMPC_MINI

typedef struct {
	gsize offset;
	GTime t; /* GTime is equivalent to time_t */
} scroll_state_t;

char *strscroll(char *str, char *separator, int width, scroll_state_t *st);

#endif

#endif
