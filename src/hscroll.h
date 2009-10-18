/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2009 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef HSCROLL_H
#define HSCROLL_H

#include <glib.h>

struct hscroll {
	gsize offset;
};

static inline void
hscroll_reset(struct hscroll *hscroll)
{
	hscroll->offset = 0;
}

static inline void
hscroll_step(struct hscroll *hscroll)
{
	++hscroll->offset;
}

char *
strscroll(struct hscroll *hscroll, const char *str, const char *separator,
	  unsigned width);

#endif
