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

#include "plugin.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>

struct plugin_cycle {
	/** the plugin list; used for traversing to the next plugin */
	struct plugin_list *list;

	/** arguments passed to execv() */
	char **argv;

	/** caller defined callback function */
	plugin_callback_t callback;
	/** caller defined pointer passed to #callback */
	void *callback_data;

	/** the index of the next plugin which is going to be
	    invoked */
	guint next_plugin;

	/** the pid of the plugin process, or -1 if none is currently
	    running */
	pid_t pid;
	/** the pipe to the plugin process, or -1 if none is currently
	    open */
	int fd;
	/** the GLib channel of #fd */
	GIOChannel *channel;
	/** the GLib IO watch of #channel */
	guint event_id;

	/** the output of the current plugin */
	GString *data;
};

static bool
register_plugin(struct plugin_list *list, char *path)
{
	int ret;
	struct stat st;

	ret = stat(path, &st);
	if (ret < 0)
		return false;

	g_ptr_array_add(list->plugins, path);
	return true;
}

bool
plugin_list_load_directory(struct plugin_list *list, const char *path)
{
	GDir *dir;
	const char *name;
	char *plugin;
	bool ret;

	dir = g_dir_open(path, 0, NULL);
	if (dir == NULL)
		return false;

	while ((name = g_dir_read_name(dir)) != NULL) {
		plugin = g_build_filename(path, name, NULL);
		ret = register_plugin(list, plugin);
		if (!ret)
			g_free(plugin);
	}

	g_dir_close(dir);
	return true;
}

void plugin_list_deinit(struct plugin_list *list)
{
	for (guint i = 0; i < list->plugins->len; ++i)
		free(g_ptr_array_index(list->plugins, i));
	g_ptr_array_free(list->plugins, TRUE);
}

static void
next_plugin(struct plugin_cycle *cycle);

static void
plugin_eof(struct plugin_cycle *cycle)
{
	int ret, status;

	g_io_channel_unref(cycle->channel);
	close(cycle->fd);
	cycle->fd = -1;

	ret = waitpid(cycle->pid, &status, 0);
	cycle->pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* the plugin has failed */
		g_string_free(cycle->data, TRUE);
		cycle->data = NULL;

		next_plugin(cycle);
	} else {
		/* success: invoke the callback */
		cycle->callback(cycle->data, cycle->callback_data);
	}
}

static gboolean
plugin_data(G_GNUC_UNUSED GIOChannel *source,
	    G_GNUC_UNUSED GIOCondition condition, gpointer data)
{
	struct plugin_cycle *cycle = data;
	char buffer[256];
	ssize_t nbytes;

	assert(cycle != NULL);
	assert(cycle->fd >= 0);
	assert(cycle->pid > 0);
	assert(source == cycle->channel);

	nbytes = condition & G_IO_IN
		? read(cycle->fd, buffer, sizeof(buffer))
		: 0;
	if (nbytes <= 0) {
		plugin_eof(cycle);
		return FALSE;
	}

	g_string_append_len(cycle->data, buffer, nbytes);
	return TRUE;
}

/**
 * This is a timer callback which calls the plugin callback "some time
 * later".  This solves the problem that plugin_run() may fail
 * immediately, leaving its return value in an undefined state.
 * Instead, install a timer which calls the plugin callback in the
 * moment after.
 */
static gboolean
plugin_delayed_fail(gpointer data)
{
	struct plugin_cycle *cycle = data;

	assert(cycle != NULL);
	assert(cycle->fd < 0);
	assert(cycle->pid < 0);
	assert(cycle->data == NULL);

	cycle->callback(NULL, cycle->callback_data);

	return FALSE;
}

static int
start_plugin(struct plugin_cycle *cycle, const char *plugin_path)
{
	int ret, fds[2];
	pid_t pid;

	assert(cycle != NULL);
	assert(cycle->pid < 0);
	assert(cycle->fd < 0);
	assert(cycle->data == NULL);

	/* set new program name, but free the one from the previous
	   plugin */
	g_free(cycle->argv[0]);
	cycle->argv[0] = g_path_get_basename(plugin_path);

	ret = pipe(fds);
	if (ret < 0)
		return -1;

	pid = fork();

	if (pid < 0) {
		close(fds[0]);
		close(fds[1]);
		return -1;
	}

	if (pid == 0) {
		dup2(fds[1], 1);
		dup2(fds[1], 1);
		close(fds[0]);
		close(fds[1]);
		close(0);
		/* XXX close other fds? */

		execv(plugin_path, cycle->argv);
		_exit(1);
	}

	close(fds[1]);

	cycle->pid = pid;
	cycle->fd = fds[0];
	cycle->data = g_string_new(NULL);

	/* XXX CLOEXEC? */

	cycle->channel = g_io_channel_unix_new(cycle->fd);
	cycle->event_id = g_io_add_watch(cycle->channel, G_IO_IN|G_IO_HUP,
					 plugin_data, cycle);

	return 0;
}

static void
next_plugin(struct plugin_cycle *cycle)
{
	const char *plugin_path;
	int ret = -1;

	assert(cycle->pid < 0);
	assert(cycle->fd < 0);
	assert(cycle->data == NULL);

	if (cycle->next_plugin >= cycle->list->plugins->len) {
		/* no plugins left */
		g_timeout_add(0, plugin_delayed_fail, cycle);
		return;
	}

	plugin_path = g_ptr_array_index(cycle->list->plugins,
					cycle->next_plugin++);
	ret = start_plugin(cycle, plugin_path);
	if (ret < 0) {
		/* system error */
		g_timeout_add(0, plugin_delayed_fail, cycle);
		return;
	}
}

static char **
make_argv(const char*const* args)
{
	unsigned num = 0;
	char **ret;

	while (args[num] != NULL)
		++num;
	num += 2;

	ret = g_new(char*, num);

	/* reserve space for the program name */
	*ret++ = NULL;

	do {
		*ret++ = g_strdup(*args++);
	} while (*args != NULL);

	/* end of argument vector */
	*ret++ = NULL;

	return ret - num;
}

struct plugin_cycle *
plugin_run(struct plugin_list *list, const char *const*args,
	   plugin_callback_t callback, void *callback_data)
{
	struct plugin_cycle *cycle = g_new(struct plugin_cycle, 1);

	assert(args != NULL);

	cycle->list = list;
	cycle->argv = make_argv(args);
	cycle->callback = callback;
	cycle->callback_data = callback_data;
	cycle->next_plugin = 0;
	cycle->pid = -1;
	cycle->fd = -1;
	cycle->data = NULL;

	next_plugin(cycle);

	return cycle;
}

void
plugin_stop(struct plugin_cycle *cycle)
{
	if (cycle->fd >= 0) {
		/* close the pipe to the plugin */
		g_source_remove(cycle->event_id);
		g_io_channel_unref(cycle->channel);
		close(cycle->fd);
	}

	if (cycle->pid > 0) {
		/* kill the plugin process */
		int status;

		kill(cycle->pid, SIGTERM);
		waitpid(cycle->pid, &status, 0);
	}

	if (cycle->data != NULL)
		/* free data that has been received */
		g_string_free(cycle->data, TRUE);

	/* free argument list */
	for (guint i = 0; i == 0 || cycle->argv[i] != NULL; ++i)
		g_free(cycle->argv[i]);
	g_free(cycle->argv);

	g_free(cycle);
}
