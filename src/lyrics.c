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

#include "lyrics.h"
#include "gcc.h"

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>

static GPtrArray *plugins;

struct lyrics_loader {
	char *artist, *title;

	lyrics_callback_t callback;
	void *callback_data;

	guint next_plugin;

	pid_t pid;
	int fd;
	GIOChannel *channel;
	guint event_id;

	GString *data;
};

static int lyrics_register_plugin(const char *path0)
{
	int ret;
	struct stat st;
	char *path;

	ret = stat(path0, &st);
	if (ret < 0)
		return -1;

	path = g_strdup(path0);
	g_ptr_array_add(plugins, path);
	return 0;
}

void lyrics_init(void)
{
	plugins = g_ptr_array_new();

	/* XXX configurable paths */
	lyrics_register_plugin("./lyrics/hd.py");
	lyrics_register_plugin("./lyrics/leoslyrics.py");
	lyrics_register_plugin("./lyrics/lyricswiki.rb");
}

void lyrics_deinit(void)
{
	guint i;

	for (i = 0; i < plugins->len; ++i)
		free(g_ptr_array_index(plugins, i));
	g_ptr_array_free(plugins, TRUE);
}

static void
lyrics_next_plugin(struct lyrics_loader *loader);

static void
lyrics_eof(struct lyrics_loader *loader)
{
	int ret, status;

	g_io_channel_unref(loader->channel);
	close(loader->fd);
	loader->fd = -1;

	ret = waitpid(loader->pid, &status, 0);
	loader->pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		g_string_free(loader->data, TRUE);
		loader->data = NULL;

		lyrics_next_plugin(loader);
	} else {
		loader->callback(loader->data, loader->callback_data);
	}
}

static gboolean
lyrics_data(mpd_unused GIOChannel *source,
	    mpd_unused GIOCondition condition, gpointer data)
{
	struct lyrics_loader *loader = data;
	char buffer[256];
	ssize_t nbytes;

	assert(loader != NULL);
	assert(loader->fd >= 0);
	assert(loader->pid > 0);
	assert(source == loader->channel);

	if ((condition & G_IO_IN) == 0) {
		lyrics_eof(loader);
		return FALSE;
	}

	nbytes = condition & G_IO_IN
		? read(loader->fd, buffer, sizeof(buffer))
		: 0;
	if (nbytes <= 0) {
		lyrics_eof(loader);
		return FALSE;
	}

	g_string_append_len(loader->data, buffer, nbytes);
	return TRUE;
}

/**
 * This is a timer callback which calls the lyrics callback "some
 * timer later".  This solves the problem that lyrics_load() may fail
 * immediately, leaving its return value in an undefined state.
 * Instead, install a timer which calls the lyrics callback in the
 * moment after.
 */
static gboolean
lyrics_delayed_fail(gpointer data)
{
	struct lyrics_loader *loader = data;

	assert(loader != NULL);
	assert(loader->fd < 0);
	assert(loader->pid < 0);
	assert(loader->data == NULL);

	loader->callback(NULL, loader->callback_data);

	return FALSE;
}

static int
lyrics_start_plugin(struct lyrics_loader *loader, const char *plugin_path)
{
	int ret, fds[2];
	pid_t pid;

	assert(loader != NULL);
	assert(loader->pid < 0);
	assert(loader->fd < 0);
	assert(loader->data == NULL);

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

		execl(plugin_path, plugin_path,
		      loader->artist, loader->title, NULL);
		_exit(1);
	}

	close(fds[1]);

	loader->pid = pid;
	loader->fd = fds[0];
	loader->data = g_string_new(NULL);

	/* XXX CLOEXEC? */

	loader->channel = g_io_channel_unix_new(loader->fd);
	loader->event_id = g_io_add_watch(loader->channel, G_IO_IN|G_IO_HUP,
					  lyrics_data, loader);

	return 0;
}

static void
lyrics_next_plugin(struct lyrics_loader *loader)
{
	const char *plugin_path;
	int ret = -1;

	assert(loader->pid < 0);
	assert(loader->fd < 0);
	assert(loader->data == NULL);

	if (loader->next_plugin >= plugins->len) {
		/* no plugins left */
		g_timeout_add(0, lyrics_delayed_fail, loader);
		return;
	}

	plugin_path = g_ptr_array_index(plugins, loader->next_plugin++);
	ret = lyrics_start_plugin(loader, plugin_path);
	if (ret < 0) {
		/* system error */
		g_timeout_add(0, lyrics_delayed_fail, loader);
		return;
	}
}

struct lyrics_loader *
lyrics_load(const char *artist, const char *title,
	    lyrics_callback_t callback, void *data)
{
	struct lyrics_loader *loader = g_new(struct lyrics_loader, 1);

	assert(artist != NULL);
	assert(title != NULL);

	if (loader == NULL)
		return NULL;

	loader->artist = g_strdup(artist);
	loader->title = g_strdup(title);
	loader->callback = callback;
	loader->data = data;
	loader->next_plugin = 0;
	loader->pid = -1;
	loader->fd = -1;
	loader->data = NULL;

	lyrics_next_plugin(loader);

	return loader;
}

void
lyrics_free(struct lyrics_loader *loader)
{
	if (loader->fd >= 0) {
		g_source_remove(loader->event_id);
		g_io_channel_unref(loader->channel);
		close(loader->fd);
	}

	if (loader->pid > 0) {
		int status;

		kill(loader->pid, SIGTERM);
		waitpid(loader->pid, &status, 0);
	}

	if (loader->data != NULL)
		g_string_free(loader->data, TRUE);
}
