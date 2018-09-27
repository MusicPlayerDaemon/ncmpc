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

#include "plugin.hxx"
#include "io/Path.hxx"
#include "util/Compiler.h"
#include "util/UriUtil.hxx"

#include <glib.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <algorithm>
#include <memory>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/wait.h>

struct PluginCycle;

struct PluginPipe {
	PluginCycle *cycle;

	/** the pipe to the plugin process */
	boost::asio::posix::stream_descriptor fd;

	/** the output of the current plugin */
	std::string data;

	std::array<char, 256> buffer;

	PluginPipe(boost::asio::io_service &io_service) noexcept
		:fd(io_service) {}

	~PluginPipe() noexcept {
		Close();
	}

	void AsyncRead() noexcept {
		fd.async_read_some(boost::asio::buffer(buffer),
				   std::bind(&PluginPipe::OnRead, this,
					     std::placeholders::_1,
					     std::placeholders::_2));
	}

	void OnRead(const boost::system::error_code &error,
		    std::size_t bytes_transferred) noexcept;

	void Close() noexcept {
		if (!fd.is_open())
			return;

		fd.cancel();
		fd.close();
	}
};

struct PluginCycle {
	/** the plugin list; used for traversing to the next plugin */
	PluginList *list;

	/** arguments passed to execv() */
	std::unique_ptr<char *[]> argv;

	/** caller defined callback function */
	plugin_callback_t callback;
	/** caller defined pointer passed to #callback */
	void *callback_data;

	/** the index of the next plugin which is going to be
	    invoked */
	guint next_plugin = 0;

	/** the pid of the plugin process, or -1 if none is currently
	    running */
	pid_t pid = -1;

	/** the stdout pipe */
	PluginPipe pipe_stdout;
	/** the stderr pipe */
	PluginPipe pipe_stderr;

	/** list of all error messages from failed plugins */
	std::string all_errors;

	boost::asio::steady_timer delayed_fail_timer;

	PluginCycle(boost::asio::io_service &io_service,
		    PluginList &_list, std::unique_ptr<char *[]> &&_argv,
		    plugin_callback_t _callback, void *_callback_data) noexcept
		:list(&_list), argv(std::move(_argv)),
		 callback(_callback), callback_data(_callback_data),
		 pipe_stdout(io_service), pipe_stderr(io_service),
		 delayed_fail_timer(io_service) {}

	void ScheduleDelayedFail() noexcept {
		boost::system::error_code error;
		delayed_fail_timer.expires_from_now(std::chrono::seconds(0),
						    error);
		delayed_fail_timer.async_wait(std::bind(&PluginCycle::OnDelayedFail,
							this,
							std::placeholders::_1));
	}

private:
	void OnDelayedFail(const boost::system::error_code &error) noexcept;
};

static bool
register_plugin(PluginList *list, std::string &&path) noexcept
{
	struct stat st;
	if (stat(path.c_str(), &st) < 0)
		return false;

	list->plugins.emplace_back(std::move(path));
	return true;
}

bool
plugin_list_load_directory(PluginList *list, const char *path) noexcept
{
	GDir *dir = g_dir_open(path, 0, nullptr);
	if (dir == nullptr)
		return false;

	const char *name;
	while ((name = g_dir_read_name(dir)) != nullptr) {
		register_plugin(list, BuildPath(path, name));
	}

	g_dir_close(dir);

	std::sort(list->plugins.begin(), list->plugins.end());

	return true;
}

static void
next_plugin(PluginCycle *cycle) noexcept;

static void
plugin_eof(PluginCycle *cycle, PluginPipe *p) noexcept
{
	p->fd.close();

	/* Only if both pipes are have EOF status we are done */
	if (cycle->pipe_stdout.fd.is_open() || cycle->pipe_stderr.fd.is_open())
		return;

	int status, ret = waitpid(cycle->pid, &status, 0);
	cycle->pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* If we encountered an error other than service unavailable
		 * (69), log it for later. If all plugins fail, we may get
		 * some hints for debugging.*/
		if (!cycle->pipe_stderr.data.empty() &&
		    WEXITSTATUS(status) != 69) {
			cycle->all_errors += "*** ";
			cycle->all_errors += cycle->argv[0];
			cycle->all_errors += " ***\n\n";
			cycle->all_errors += cycle->pipe_stderr.data;
			cycle->all_errors += "\n";
		}

		/* the plugin has failed */
		cycle->pipe_stdout.data.clear();
		cycle->pipe_stderr.data.clear();

		next_plugin(cycle);
	} else {
		/* success: invoke the callback */
		cycle->callback(std::move(cycle->pipe_stdout.data), true,
				cycle->argv[0], cycle->callback_data);
	}
}

void
PluginPipe::OnRead(const boost::system::error_code &error,
		   std::size_t bytes_transferred) noexcept
{
	if (error) {
		if (error == boost::asio::error::operation_aborted)
			/* this object has already been deleted; bail out
			   quickly without touching anything */
			return;

		plugin_eof(cycle, this);
		return;
	}

	data.append(&buffer.front(), bytes_transferred);
	AsyncRead();
}

/**
 * This is a timer callback which calls the plugin callback "some time
 * later".  This solves the problem that plugin_run() may fail
 * immediately, leaving its return value in an undefined state.
 * Instead, install a timer which calls the plugin callback in the
 * moment after.
 */
void
PluginCycle::OnDelayedFail(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

	assert(!pipe_stdout.fd.is_open());
	assert(!pipe_stderr.fd.is_open());
	assert(pid < 0);

	callback(std::move(all_errors), false, nullptr,
		 callback_data);
}

static void
plugin_fd_add(PluginCycle *cycle, PluginPipe *p, int fd) noexcept
{
	p->cycle = cycle;
	p->fd.assign(fd);
	p->AsyncRead();
}

static int
start_plugin(PluginCycle *cycle, const char *plugin_path) noexcept
{
	assert(cycle != nullptr);
	assert(cycle->pid < 0);
	assert(!cycle->pipe_stdout.fd.is_open());
	assert(!cycle->pipe_stderr.fd.is_open());
	assert(cycle->pipe_stdout.data.empty());
	assert(cycle->pipe_stderr.data.empty());

	/* set new program name, but free the one from the previous
	   plugin */
	cycle->argv[0] = const_cast<char *>(GetUriFilename(plugin_path));

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

		execv(plugin_path, cycle->argv.get());
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
next_plugin(PluginCycle *cycle) noexcept
{
	assert(cycle->pid < 0);
	assert(!cycle->pipe_stdout.fd.is_open());
	assert(!cycle->pipe_stderr.fd.is_open());
	assert(cycle->pipe_stdout.data.empty());
	assert(cycle->pipe_stderr.data.empty());

	if (cycle->next_plugin >= cycle->list->plugins.size()) {
		/* no plugins left */
		cycle->ScheduleDelayedFail();
		return;
	}

	const char *plugin_path = (const char *)
		cycle->list->plugins[cycle->next_plugin++].c_str();
	if (start_plugin(cycle, plugin_path) < 0) {
		/* system error */
		cycle->ScheduleDelayedFail();
		return;
	}
}

static auto
make_argv(const char*const* args) noexcept
{
	unsigned num = 0;
	while (args[num] != nullptr)
		++num;
	num += 2;

	std::unique_ptr<char *[]> result(new char *[num]);

	char **ret = result.get();

	/* reserve space for the program name */
	*ret++ = nullptr;

	while (*args != nullptr)
		*ret++ = const_cast<char *>(*args++);

	/* end of argument vector */
	*ret++ = nullptr;

	return result;
}

PluginCycle *
plugin_run(boost::asio::io_service &io_service,
	   PluginList *list, const char *const*args,
	   plugin_callback_t callback, void *callback_data) noexcept
{
	assert(args != nullptr);

	auto *cycle = new PluginCycle(io_service, *list, make_argv(args),
				      callback, callback_data);
	next_plugin(cycle);

	return cycle;
}

void
plugin_stop(PluginCycle *cycle) noexcept
{
	if (cycle->pid > 0) {
		/* kill the plugin process */

		cycle->pipe_stdout.Close();
		cycle->pipe_stderr.Close();

		int status;

		kill(cycle->pid, SIGTERM);
		waitpid(cycle->pid, &status, 0);
	}

	delete cycle;
}
