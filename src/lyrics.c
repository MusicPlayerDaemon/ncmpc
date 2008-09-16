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

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <pthread.h>

static GPtrArray *plugins;

struct lyrics_loader {
	char *artist, *title;

	enum lyrics_loader_result result;

	pthread_t thread;
	pthread_mutex_t mutex;

	pid_t pid;
	int fd;

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

static int
lyrics_start_plugin(struct lyrics_loader *loader, const char *plugin_path)
{
	int ret, fds[2];
	pid_t pid;

	assert(loader != NULL);
	assert(loader->result == LYRICS_BUSY);
	assert(loader->pid < 0);

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

	return 0;
}

static int
lyrics_try_plugin(struct lyrics_loader *loader, const char *plugin_path)
{
	int ret, status;
	char buffer[256];
	ssize_t nbytes;

	assert(loader != NULL);
	assert(loader->fd >= 0);

	ret = lyrics_start_plugin(loader, plugin_path);
	if (ret != 0)
		return ret;

	assert(loader->pid > 0);

	while ((nbytes = read(loader->fd, buffer, sizeof(buffer))) > 0)
		g_string_append_len(loader->data, buffer, nbytes);

	ret = waitpid(loader->pid, &status, 0);
	loader->pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		g_string_free(loader->data, TRUE);
		return -1;
	}

	return 0;
}

static void *
lyrics_thread(void *arg)
{
	struct lyrics_loader *loader = arg;
	guint next_plugin = 0;
	int ret = -1;

	while (next_plugin < plugins->len && ret != 0) {
		const char *plugin_path = g_ptr_array_index(plugins,
							    next_plugin++);
		ret = lyrics_try_plugin(loader, plugin_path);
		assert(loader->pid < 0);
	}

	pthread_mutex_lock(&loader->mutex);
	loader->result = ret == 0 ? LYRICS_SUCCESS : LYRICS_FAILED;
	loader->thread = 0;
	pthread_mutex_unlock(&loader->mutex);
	return NULL;
}

struct lyrics_loader *
lyrics_load(const char *artist, const char *title)
{
	struct lyrics_loader *loader = g_new(struct lyrics_loader, 1);
	int ret;

	assert(artist != NULL);
	assert(title != NULL);

	if (loader == NULL)
		return NULL;

	loader->artist = g_strdup(artist);
	loader->title = g_strdup(title);
	loader->result = LYRICS_BUSY;
	loader->pid = -1;

	pthread_mutex_init(&loader->mutex, NULL);

	ret = pthread_create(&loader->thread, NULL, lyrics_thread, loader);
	if (ret != 0) {
		lyrics_free(loader);
		return NULL;
	}

	return loader;
}

void
lyrics_free(struct lyrics_loader *loader)
{
	pid_t pid = loader->pid;
	pthread_t thread = loader->thread;

	if (pid > 0)
		kill(pid, SIGTERM);

	if (loader->thread != 0)
		pthread_join(thread, NULL);

	assert(loader->pid < 0);
	assert(loader->thread == 0);

	if (loader->result == LYRICS_SUCCESS && loader->data != NULL)
		g_string_free(loader->data, TRUE);
}

enum lyrics_loader_result
lyrics_result(struct lyrics_loader *loader)
{
	return loader->result;
}

const GString *
lyrics_get(struct lyrics_loader *loader)
{
	/* sync with thread */
	pthread_mutex_lock(&loader->mutex);
	pthread_mutex_unlock(&loader->mutex);

	assert(loader->result == LYRICS_SUCCESS);
	assert(loader->pid < 0);
	assert(loader->thread == 0);
	assert(loader->data != NULL);

	return loader->data;
}
