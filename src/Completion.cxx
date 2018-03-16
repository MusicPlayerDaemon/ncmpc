/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "Completion.hxx"

#include <string.h>

/**
 * Wrapper for strncmp().  We are not allowed to pass &strncmp to
 * g_completion_set_compare(), because strncmp() takes size_t where
 * g_completion_set_compare passes a gsize value.
 */
static gint
completion_strncmp(const gchar *s1, const gchar *s2, gsize n)
{
	return strncmp(s1, s2, n);
}

Completion::Completion()
	:gcmp(g_completion_new(nullptr))
{
	g_completion_set_compare(gcmp, completion_strncmp);
}

Completion::~Completion()
{
	g_completion_free(gcmp);
}
