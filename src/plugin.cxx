// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "plugin.hxx"
#include "io/Path.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "event/PipeEvent.hxx"
#include "event/CoarseTimerEvent.hxx"
#include "util/ScopeExit.hxx"
#include "util/UriUtil.hxx"

#include <algorithm>
#include <memory>

#include <assert.h>
#include <stdlib.h>
#include <spawn.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

struct PluginCycle;

class PluginPipe {
	PluginCycle &cycle;

	/** the pipe to the plugin process */
	PipeEvent socket_event;

	/** the output of the current plugin */
	std::string data;

public:
	PluginPipe(EventLoop &event_loop,
		   PluginCycle &_cycle) noexcept
		:cycle(_cycle),
		 socket_event(event_loop, BIND_THIS_METHOD(OnRead)) {}

	~PluginPipe() noexcept {
		socket_event.Close();
	}

	void Start(UniqueFileDescriptor &&fd) noexcept {
		socket_event.Open(fd.Release());
		socket_event.ScheduleRead();
	}

	void Close() noexcept {
		socket_event.Close();
	}

	bool IsOpen() const noexcept {
		return socket_event.IsDefined();
	}

	bool IsEmpty() const noexcept {
		return data.empty();
	}

	std::string TakeData() noexcept {
		return std::move(data);
	}

	void Clear() {
		data.clear();
	}

private:
	void OnRead(unsigned events) noexcept;
};

struct PluginCycle {
	/** the plugin list; used for traversing to the next plugin */
	const PluginList &list;

	/** arguments passed to execv() */
	std::unique_ptr<char *[]> argv;

	/** caller defined handler object */
	PluginResponseHandler &handler;

	/** the index of the next plugin which is going to be
	    invoked */
	unsigned next_plugin = 0;

	/** the pid of the plugin process, or -1 if none is currently
	    running */
	pid_t pid = -1;

	/** the stdout pipe */
	PluginPipe pipe_stdout;
	/** the stderr pipe */
	PluginPipe pipe_stderr;

	/** list of all error messages from failed plugins */
	std::string all_errors;

	CoarseTimerEvent delayed_fail_timer;

	PluginCycle(EventLoop &event_loop,
		    const PluginList &_list, std::unique_ptr<char *[]> &&_argv,
		    PluginResponseHandler &_handler) noexcept
		:list(_list), argv(std::move(_argv)),
		 handler(_handler),
		 pipe_stdout(event_loop, *this),
		 pipe_stderr(event_loop, *this),
		 delayed_fail_timer(event_loop, BIND_THIS_METHOD(OnDelayedFail)) {}

	void TryNextPlugin() noexcept;

	void Stop() noexcept;

	void ScheduleDelayedFail() noexcept {
		delayed_fail_timer.Schedule(std::chrono::seconds(0));
	}

	void OnEof() noexcept;

private:
	int LaunchPlugin(const char *plugin_path) noexcept;

	void OnDelayedFail() noexcept;
};

static bool
register_plugin(PluginList &list, std::string &&path) noexcept
{
	struct stat st;
	if (stat(path.c_str(), &st) < 0)
		return false;

	list.plugins.emplace_back(std::move(path));
	return true;
}

static constexpr bool
ShallSkipDirectoryEntry(const char *name) noexcept
{
	return name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0));
}

PluginList
plugin_list_load_directory(const char *path) noexcept
{
	PluginList list;

	DIR *dir = opendir(path);
	if (dir == nullptr)
		return list;

	AtScopeExit(dir) { closedir(dir); };

	while (const auto *e = readdir(dir)) {
		const char *name = e->d_name;
		if (!ShallSkipDirectoryEntry(name))
			register_plugin(list, BuildPath(path, name));
	}

	std::sort(list.plugins.begin(), list.plugins.end());

	return list;
}

void
PluginCycle::OnEof() noexcept
{
	/* Only if both pipes are have EOF status we are done */
	if (pipe_stdout.IsOpen() || pipe_stderr.IsOpen())
		return;

	int status, ret = waitpid(pid, &status, 0);
	pid = -1;

	if (ret < 0 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
		/* If we encountered an error other than service unavailable
		 * (69), log it for later. If all plugins fail, we may get
		 * some hints for debugging.*/
		const auto data = pipe_stderr.TakeData();
		if (!data.empty() && WEXITSTATUS(status) != 69) {
			all_errors += "*** ";
			all_errors += argv[0];
			all_errors += " ***\n\n";
			all_errors += data;
			all_errors += "\n";
		}

		/* the plugin has failed */
		pipe_stdout.Clear();
		pipe_stderr.Clear();

		TryNextPlugin();
	} else {
		/* success: invoke the callback */
		handler.OnPluginSuccess(argv[0], pipe_stdout.TakeData());
	}
}

void
PluginPipe::OnRead(unsigned) noexcept
{
	char buffer[256];
	ssize_t nbytes = socket_event.GetFileDescriptor().Read(buffer,
							       sizeof(buffer));
	if (nbytes <= 0) {
		socket_event.Close();
		cycle.OnEof();
		return;
	}

	data.append(buffer, nbytes);
	socket_event.ScheduleRead();
}

/**
 * This is a timer callback which calls the plugin callback "some time
 * later".  This solves the problem that plugin_run() may fail
 * immediately, leaving its return value in an undefined state.
 * Instead, install a timer which calls the plugin callback in the
 * moment after.
 */
void
PluginCycle::OnDelayedFail() noexcept
{
	assert(!pipe_stdout.IsOpen());
	assert(!pipe_stderr.IsOpen());
	assert(pid < 0);

	handler.OnPluginError(std::move(all_errors));
}

int
PluginCycle::LaunchPlugin(const char *plugin_path) noexcept
{
	assert(pid < 0);
	assert(!pipe_stdout.IsOpen());
	assert(!pipe_stderr.IsOpen());
	assert(pipe_stdout.IsEmpty());
	assert(pipe_stderr.IsEmpty());

	/* set new program name, but free the one from the previous
	   plugin */
	argv[0] = const_cast<char *>(GetUriFilename(plugin_path));

	UniqueFileDescriptor stdout_r, stdout_w;
	UniqueFileDescriptor stderr_r, stderr_w;
	if (!UniqueFileDescriptor::CreatePipe(stdout_r, stdout_w) ||
	    !UniqueFileDescriptor::CreatePipe(stderr_r, stderr_w))
		return -1;

	posix_spawn_file_actions_t file_actions;
	posix_spawn_file_actions_init(&file_actions);
	AtScopeExit(&file_actions) { posix_spawn_file_actions_destroy(&file_actions); };

	posix_spawn_file_actions_addclose(&file_actions, STDIN_FILENO);
	posix_spawn_file_actions_adddup2(&file_actions, stdout_w.Get(),
					 STDOUT_FILENO);
	posix_spawn_file_actions_adddup2(&file_actions, stderr_w.Get(),
					 STDERR_FILENO);

	if (posix_spawn(&pid, plugin_path, &file_actions, nullptr,
			argv.get(), environ) != 0)
		return -1;

	pipe_stdout.Start(std::move(stdout_r));
	pipe_stderr.Start(std::move(stderr_r));

	return 0;
}

void
PluginCycle::TryNextPlugin() noexcept
{
	assert(pid < 0);
	assert(!pipe_stdout.IsOpen());
	assert(!pipe_stderr.IsOpen());
	assert(pipe_stdout.IsEmpty());
	assert(pipe_stderr.IsEmpty());

	if (next_plugin >= list.plugins.size()) {
		/* no plugins left */
		ScheduleDelayedFail();
		return;
	}

	const char *plugin_path = (const char *)
		list.plugins[next_plugin++].c_str();
	if (LaunchPlugin(plugin_path) < 0) {
		/* system error */
		ScheduleDelayedFail();
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
plugin_run(EventLoop &event_loop,
	   const PluginList &list, const char *const*args,
	   PluginResponseHandler &handler) noexcept
{
	assert(args != nullptr);

	auto *cycle = new PluginCycle(event_loop, list, make_argv(args),
				      handler);
	cycle->TryNextPlugin();

	return cycle;
}

void
PluginCycle::Stop() noexcept
{
	if (pid > 0) {
		/* kill the plugin process */

		pipe_stdout.Close();
		pipe_stderr.Close();

		int status;

		kill(pid, SIGTERM);
		waitpid(pid, &status, 0);
	}

	delete this;
}

void
plugin_stop(PluginCycle &cycle) noexcept
{
	cycle.Stop();
}
