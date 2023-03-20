// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>
#include <string>

class EventLoop;

/**
 * When a plugin cycle is finished, one method of this class is called.  In any
 * case, plugin_stop() has to be called to free all memory.
 */
class PluginResponseHandler {
public:
	/**
	 * @param plugin_name the name of the plugin which succeeded
	 * @param result the plugin's output (stdout)
	 */
	virtual void OnPluginSuccess(const char *plugin_name,
				     std::string result) noexcept = 0;

	virtual void OnPluginError(std::string error) noexcept = 0;
};

/**
 * A list of registered plugins.
 */
struct PluginList {
	std::vector<std::string> plugins;
};

/**
 * This object represents a cycle through all available plugins, until
 * a plugin returns a positive result.
 */
struct PluginCycle;

/**
 * Load all plugins (executables) in a directory.
 */
PluginList
plugin_list_load_directory(const char *path) noexcept;

/**
 * Run plugins in this list, until one returns success (or until the
 * plugin list is exhausted).
 *
 * @param list the plugin list
 * @param args nullptr terminated command line arguments passed to the
 * plugin programs; they must remain valid while the plugin runs
 * @param handler the handler which will be called when a result is
 * available
 */
PluginCycle *
plugin_run(EventLoop &event_loop,
	   const PluginList &list, const char *const*args,
	   PluginResponseHandler &handler) noexcept;

/**
 * Stops the plugin cycle and frees resources.  This can be called to
 * abort the current cycle, or after the plugin_callback_t has been
 * invoked.
 */
void
plugin_stop(PluginCycle &invocation) noexcept;

#endif
