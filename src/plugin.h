/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2017 The Music Player Daemon Project
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <glib.h>
#include <stdbool.h>

/**
 * A list of registered plugins.
 */
struct plugin_list {
	GPtrArray *plugins;
};

/**
 * When a plugin cycle is finished, this function is called.  In any
 * case, plugin_stop() has to be called to free all memory.
 *
 * @param result the plugin's output (stdout) on success; summary of all error
 * messages on failure as determined by success
 * @param success result of the plugin cycle; true if result is meaningful
 * output, false if result contains error messages
 * @param plugin_name the name of the plugin which succeeded; becomes invalid
 * when plugin_stop is called (i.e. strdup it if you need it afterwards).
 * @param data the caller defined pointer passed to plugin_run()
 */
typedef void (*plugin_callback_t)(const GString *result, const bool success,
				  const char *plugin_name, void *data);

/**
 * This object represents a cycle through all available plugins, until
 * a plugin returns a positive result.
 */
struct plugin_cycle;

/**
 * Initialize an empty plugin_list structure.
 */
static inline void
plugin_list_init(struct plugin_list *list)
{
	list->plugins = g_ptr_array_new();
}

/**
 * Load all plugins (executables) in a directory.
 */
bool
plugin_list_load_directory(struct plugin_list *list, const char *path);

/**
 * Frees all memory held by the plugin_list object (but not the
 * pointer itself).
 */
void plugin_list_deinit(struct plugin_list *list);

/**
 * Run plugins in this list, until one returns success (or until the
 * plugin list is exhausted).
 *
 * @param list the plugin list
 * @param args NULL terminated command line arguments passed to the
 * plugin programs
 * @param callback the callback function which will be called when a
 * result is available
 * @param callback_data caller defined pointer which is passed to the
 * callback function
 */
struct plugin_cycle *
plugin_run(struct plugin_list *list, const char *const*args,
	   plugin_callback_t callback, void *callback_data);

/**
 * Stops the plugin cycle and frees resources.  This can be called to
 * abort the current cycle, or after the plugin_callback_t has been
 * invoked.
 */
void
plugin_stop(struct plugin_cycle *invocation);

#endif
