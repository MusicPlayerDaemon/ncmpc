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

#include "plugin.h"
#include "Compiler.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>

struct plugin_pipe {
	struct plugin_cycle *cycle;

	/** the pipe to the plugin process, or -1 if none is currently
	    open */
	int fd;
	/** the GLib IO watch of #fd */
	guint event_id;
	/** the output of the current plugin */
	GString *data;
};

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

	/** the stdout pipe */
	struct plugin_pipe pipe_stdout;
	/** the stderr pipe */
	struct plugin_pipe pipe_stderr;

	/** list of all error messages from failed plugins */
	GString *all_errors;
};

static bool
register_plugin(struct plugin_list *list, char *path)
{
	struct stat st;
	if (stat(path, &st) < 0)
		return false;

	g_ptr_array_add(list->plugins, path);
	return true;
}

static gint
plugin_compare_func_alpha(gconstpointer plugin1, gconstpointer plugin2)
{
	return strcmp(* (char * const *) plugin1, * (char * const *) plugin2);
}

static void
plugin_list_sort(struct plugin_list *list, GCompareFunc compare_func)
{
	g_ptr_array_sort(list->plugins,
			compare_func);
}

bool
plugin_list_load_directory(struct plugin_list *list, const char *path)
{
	GDir *dir = g_dir_open(path, 0, NULL);
	if (dir == NULL)
		return false;

	const char *name;
	while ((name = g_dir_read_name(dir)) != NULL) {
		char *plugin = g_build_filename(path, name, NULL);
		if (!register_plugin(list, plugin))
			g_free(plugin);
	}

	g_dir_close(dir);

	plugin_list_sort(list, plugin_compare_func_alpha);

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
plugin_eof(struct plugin_cycle *cycle, struct plugin_pipe *p)
{
	close(p->fd);
	p->fd = -1;

	/* Only if both pipes are have EOF status we are done */
	if (cycle->pipe_stdout.fd != -1 || cycle->pipe_stderr.fd != -1)
		return;

	int status, ret = waitpid(cycle->pid, &status, 0);
	cycle->pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* If we encountered an error other than service unavailable
		 * (69), log it for later. If all plugins fail, we may get
		 * some hints for debugging.*/
		if (cycle->pipe_stderr.data->len > 0 &&
		    WEXITSTATUS(status) != 69)
			g_string_append_printf(cycle->all_errors,
					       "*** %s ***\n\n%s\n",
					       cycle->argv[0],
					       cycle->pipe_stderr.data->str);

		/* the plugin has failed */
		g_string_free(cycle->pipe_stdout.data, TRUE);
		cycle->pipe_stdout.data = NULL;
		g_string_free(cycle->pipe_stderr.data, TRUE);
		cycle->pipe_stderr.data = NULL;

		next_plugin(cycle);
	} else {
		/* success: invoke the callback */
		cycle->callback(cycle->pipe_stdout.data, true,
				cycle->argv[0], cycle->callback_data);
	}
}

static gboolean
plugin_data(gcc_unused GIOChannel *source,
	    gcc_unused GIOCondition condition, gpointer data)
{
	struct plugin_pipe *p = data;
	assert(p->fd >= 0);

	struct plugin_cycle *cycle = p->cycle;
	assert(cycle != NULL);
	assert(cycle->pid > 0);

	char buffer[256];
	ssize_t nbytes = condition & G_IO_IN
		? read(p->fd, buffer, sizeof(buffer))
		: 0;
	if (nbytes <= 0) {
		plugin_eof(cycle, p);
		return FALSE;
	}

	g_string_append_len(p->data, buffer, nbytes);
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
	assert(cycle->pipe_stdout.fd < 0);
	assert(cycle->pipe_stderr.fd < 0);
	assert(cycle->pid < 0);

	cycle->callback(cycle->all_errors, false, NULL, cycle->callback_data);

	return FALSE;
}

static void
plugin_fd_add(struct plugin_cycle *cycle, struct plugin_pipe *p, int fd)
{
	p->cycle = cycle;
	p->fd = fd;
	p->data = g_string_new(NULL);
	GIOChannel *channel = g_io_channel_unix_new(fd);
	p->event_id = g_io_add_watch(channel, G_IO_IN|G_IO_HUP,
				     plugin_data, cycle);
	g_io_channel_unref(channel);
}

static int
start_plugin(struct plugin_cycle *cycle, const char *plugin_path)
{
	assert(cycle != NULL);
	assert(cycle->pid < 0);
	assert(cycle->pipe_stdout.fd < 0);
	assert(cycle->pipe_stderr.fd < 0);
	assert(cycle->pipe_stdout.data == NULL);
	assert(cycle->pipe_stderr.data == NULL);

	/* set new program name, but free the one from the previous
	   plugin */
	g_free(cycle->argv[0]);
	cycle->argv[0] = g_path_get_basename(plugin_path);

	int fds_stdout[2];
	if (pipe(fds_stdout) < 0)
		return -1;

	int fds_stderr[2];
	if (pipe(fds_stderr) < 0) {
		close(fds_stdout[0]);
		close(fds_stdout[1]);
		return -1;
	}

	pid_t pid = fork();

	if (pid < 0) {
		close(fds_stdout[0]);
		close(fds_stdout[1]);
		close(fds_stderr[0]);
		close(fds_stderr[1]);
		return -1;
	}

	if (pid == 0) {
		dup2(fds_stdout[1], 1);
		dup2(fds_stderr[1], 2);
		close(fds_stdout[0]);
		close(fds_stdout[1]);
		close(fds_stderr[0]);
		close(fds_stderr[1]);
		close(0);
		/* XXX close other fds? */

		execv(plugin_path, cycle->argv);
		_exit(1);
	}

	close(fds_stdout[1]);
	close(fds_stderr[1]);

	cycle->pid = pid;

	/* XXX CLOEXEC? */

	plugin_fd_add(cycle, &cycle->pipe_stdout, fds_stdout[0]);
	plugin_fd_add(cycle, &cycle->pipe_stderr, fds_stderr[0]);

	return 0;
}

static void
next_plugin(struct plugin_cycle *cycle)
{
	assert(cycle->pid < 0);
	assert(cycle->pipe_stdout.fd < 0);
	assert(cycle->pipe_stderr.fd < 0);
	assert(cycle->pipe_stdout.data == NULL);
	assert(cycle->pipe_stderr.data == NULL);

	if (cycle->next_plugin >= cycle->list->plugins->len) {
		/* no plugins left */
		g_idle_add(plugin_delayed_fail, cycle);
		return;
	}

	const char *plugin_path = g_ptr_array_index(cycle->list->plugins,
						    cycle->next_plugin++);
	if (start_plugin(cycle, plugin_path) < 0) {
		/* system error */
		g_idle_add(plugin_delayed_fail, cycle);
		return;
	}
}

static char **
make_argv(const char*const* args)
{
	unsigned num = 0;
	while (args[num] != NULL)
		++num;
	num += 2;

	char **ret = g_new(char *, num);

	/* reserve space for the program name */
	*ret++ = NULL;

	while (*args != NULL)
		*ret++ = g_strdup(*args++);

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
	cycle->pipe_stdout.fd = -1;
	cycle->pipe_stderr.fd = -1;
	cycle->pipe_stdout.data = NULL;
	cycle->pipe_stderr.data = NULL;

	cycle->all_errors = g_string_new(NULL);
	next_plugin(cycle);

	return cycle;
}

static void
plugin_fd_remove(struct plugin_pipe *p)
{
	if (p->fd >= 0) {
		g_source_remove(p->event_id);
		close(p->fd);
	}
}

void
plugin_stop(struct plugin_cycle *cycle)
{
	plugin_fd_remove(&cycle->pipe_stdout);
	plugin_fd_remove(&cycle->pipe_stderr);

	if (cycle->pid > 0) {
		/* kill the plugin process */
		int status;

		kill(cycle->pid, SIGTERM);
		waitpid(cycle->pid, &status, 0);
	}

	/* free data that has been received */
	if (cycle->pipe_stdout.data != NULL)
		g_string_free(cycle->pipe_stdout.data, TRUE);
	if (cycle->pipe_stderr.data != NULL)
		g_string_free(cycle->pipe_stderr.data, TRUE);
	if (cycle->all_errors != NULL)
		g_string_free(cycle->all_errors, TRUE);

	/* free argument list */
	for (guint i = 0; i == 0 || cycle->argv[i] != NULL; ++i)
		g_free(cycle->argv[i]);
	g_free(cycle->argv);

	g_free(cycle);
}
