/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2010 The Music Player Daemon Project
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

#include "lyrics.h"
#include "config.h"

#include <assert.h>

static struct plugin_list empty, plugins;

void lyrics_init(void)
{
	plugin_list_init(&empty);
	plugin_list_init(&plugins);
	plugin_list_load_directory(&plugins, LYRICS_PLUGIN_DIR);
}

void lyrics_deinit(void)
{
	plugin_list_deinit(&empty);
	plugin_list_deinit(&plugins);
}

struct plugin_cycle *
lyrics_load(const char *artist, const char *title,
	    plugin_callback_t callback, void *data)
{
	const char *args[3] = { artist, title, NULL };

	if (artist == NULL || title == NULL)
		return plugin_run(&empty, args, callback, data);

	return plugin_run(&plugins, args, callback, data);
}
