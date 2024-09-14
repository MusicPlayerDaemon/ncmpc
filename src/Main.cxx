// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "config.h"
#include "Instance.hxx"
#include "ncmpc.hxx"
#include "charset.hxx"
#include "Options.hxx"
#include "Command.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "ncu.hxx"
#include "screen.hxx"
#include "xterm_title.hxx"
#include "strfsong.hxx"
#include "i18n.h"
#include "util/Exception.hxx"
#include "util/PrintException.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringUTF8.hxx"

#ifndef NCMPC_MINI
#include "ConfigFile.hxx"
#endif

#include <mpd/client.h>

#include <curses.h>

#include <fmt/core.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

using std::string_view_literals::operator""sv;

ScreenManager *screen;

#ifndef NCMPC_MINI
static void
update_xterm_title(struct mpdclient &client) noexcept
{
	const struct mpd_song *song = client.GetPlayingSong();

	static constexpr std::size_t BUFSIZE = 1024;

	char tmp[BUFSIZE];
	std::string_view new_title = PACKAGE " version " VERSION ""sv;
	if (!options.xterm_title_format.empty() && song != nullptr) {
		const auto s = strfsong(tmp, options.xterm_title_format.c_str(), *song);
		if (!s.empty())
			new_title = s;
	}

	static char old_title[BUFSIZE];
	static std::size_t old_title_length = 0;

	if (new_title != std::string_view{old_title, old_title_length}) {
		std::copy(new_title.begin(), new_title.end(), old_title);
		old_title_length = new_title.size();
		set_xterm_title(new_title);
	}
}
#endif

static bool
should_enable_update_timer(struct mpdclient &client) noexcept
{
	return client.playing;
}

static void
auto_update_timer(Instance &instance) noexcept
{
	if (should_enable_update_timer(instance.GetClient()))
		instance.EnableUpdateTimer();
	else
		instance.DisableUpdateTimer();
}

void
Instance::UpdateClient() noexcept
{
	if (client.IsReady() &&
	    (client.events != 0 || client.playing))
		client.Update();

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title(client);
#endif

	screen_manager.Update(client, seek);
	client.events = (enum mpd_idle)0;
}

void
Instance::OnReconnectTimer() noexcept
{
	assert(client.IsDead());

	screen_manager.Alert(fmt::format(fmt::runtime(_("Connecting to {}")),
					 client.GetSettingsName()));
	doupdate();

	client.Connect();
}

void
Instance::OnMpdConnected() noexcept
{
#ifndef NCMPC_MINI
	/* quit if mpd is pre 0.14 - song id not supported by mpd */
	auto *connection = client.GetConnection();
	if (mpd_connection_cmp_server_version(connection, 0, 21, 0) < 0) {
		const unsigned *version =
			mpd_connection_get_server_version(connection);
		screen_manager.Alert(fmt::format(fmt::runtime(_("Error: MPD version {} is too old ({} needed)")),
						 fmt::format("{}.{}.{}"sv, version[0], version[1], version[2]),
						 "0.21.0"sv));
		client.Disconnect();
		doupdate();

		/* try again after 30 seconds */
		ScheduleReconnect(std::chrono::seconds{30});
		return;
	}
#endif

	screen_manager.status_bar.ClearMessage();
	doupdate();

	UpdateClient();

	auto_update_timer(*this);
}

void
Instance::OnMpdConnectFailed() noexcept
{
	/* try again in 5 seconds */
	ScheduleReconnect(std::chrono::seconds(5));
}

void
Instance::OnMpdConnectionLost() noexcept
{
	screen_manager.Update(client, seek);
	ScheduleReconnect(std::chrono::seconds{1});
}

void
Instance::OnMpdIdle([[maybe_unused]] unsigned events) noexcept
{
#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title(client);
#endif

	screen_manager.Update(client, seek);
	auto_update_timer(*this);
}

void
Instance::OnUpdateTimer() noexcept
{
	assert(pending_update_timer);
	pending_update_timer = false;

	UpdateClient();

	if (should_enable_update_timer(client))
		ScheduleUpdateTimer();
}

void
Instance::OnUpdateScreen() noexcept
{
	screen_manager.Update(client, seek);
	client.events = (enum mpd_idle)0;

	auto_update_timer(*this);
}

void
Instance::OnCommand(Command cmd) noexcept
{
	update_screen_event.Schedule();

	if (cmd == Command::QUIT) {
		event_loop.Break();
		return;
	}

	try {
		screen_manager.OnCommand(GetClient(), GetSeek(), cmd);
	} catch (...) {
		screen_manager.Alert(GetFullMessage(std::current_exception()));
		return;
	}

	if (cmd == Command::VOLUME_UP || cmd == Command::VOLUME_DOWN)
		/* make sure we don't update the volume yet */
		DisableUpdateTimer();
}

#ifdef HAVE_GETMOUSE

void
Instance::OnMouse(Point p, mmask_t bstate) noexcept
{
	update_screen_event.Schedule();

	try {
		screen_manager.OnMouse(GetClient(), GetSeek(), p, bstate);
	} catch (...) {
		screen_manager.Alert(GetFullMessage(std::current_exception()));
	}
}

#endif

#ifndef NCMPC_MINI
/**
 * Check the configured key bindings for errors, and display a status
 * message every 10 seconds.
 */
void
Instance::OnCheckKeyBindings() noexcept
{
	char buf[256];

	if (GetGlobalKeyBindings().Check(buf, sizeof(buf)))
		/* no error: disable this timer for the rest of this
		   process */
		return;

	screen_manager.Alert(buf);

	doupdate();

	ScheduleCheckKeyBindings();
}
#endif

int
main(int argc, const char *argv[])
try {
#ifdef ENABLE_LOCALE
	/* time and date formatting */
	setlocale(LC_TIME,"");
	/* care about sorting order etc */
	setlocale(LC_COLLATE,"");
	/* charset */
	setlocale(LC_CTYPE,"");
#ifdef HAVE_ICONV
	/* initialize charset conversions */
	charset_init();
#endif

	[[maybe_unused]] const ScopeInitUTF8 init_utf8;

	/* initialize i18n support */
#endif

#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	textdomain(GETTEXT_PACKAGE);
#endif

	/* parse command line options - 1 pass get configuration files */
	options_parse(argc, argv);

#ifndef NCMPC_MINI
	/* read configuration */
	read_configuration();

	/* check key bindings */
	GetGlobalKeyBindings().Check(nullptr, 0);
#endif

	/* set options from environment variables */
	options_env();

	/* parse command line options - 2 pass */
	options_parse(argc, argv);

	const ScopeCursesInit curses_init;

	/* create the global Instance */
	Instance instance;
	screen = &instance.GetScreenManager();

	AtScopeExit() {
		/* this must be executed after ~Instance(), so we're
		   using AtScopeExit() to do the trick */
#ifndef NCMPC_MINI
		set_xterm_title({});
#endif
		printf("\n");
	};

	/* attempt to connect */
	instance.ScheduleReconnect(std::chrono::seconds(0));

	auto_update_timer(instance);

#ifndef NCMPC_MINI
	instance.ScheduleCheckKeyBindings();
#endif

	instance.Run();
	return EXIT_SUCCESS;
} catch (...) {
	PrintException(std::current_exception());
	return EXIT_FAILURE;
}
