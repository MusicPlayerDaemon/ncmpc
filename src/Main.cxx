// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "config.h"
#include "Instance.hxx"
#include "ncmpc.hxx"
#include "mpdclient.hxx"
#include "callbacks.hxx"
#include "charset.hxx"
#include "Options.hxx"
#include "Command.hxx"
#include "Bindings.hxx"
#include "GlobalBindings.hxx"
#include "ncu.hxx"
#include "screen.hxx"
#include "screen_status.hxx"
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

static Instance *global_instance;

ScreenManager *screen;

#ifndef NCMPC_MINI
static void
update_xterm_title(struct mpdclient &client) noexcept
{
	const struct mpd_song *song = client.GetPlayingSong();

	static constexpr std::size_t BUFSIZE = 1024;

	char tmp[BUFSIZE];
	const char *new_title = nullptr;
	if (!options.xterm_title_format.empty() && song != nullptr) {
		const auto s = strfsong(tmp, options.xterm_title_format.c_str(), song);
		if (!s.empty())
			new_title = s.data();
	}

	if (new_title == nullptr)
		new_title = PACKAGE " version " VERSION;

	static char title[BUFSIZE];
	if (strncmp(title, new_title, BUFSIZE)) {
		strcpy(title, new_title);
		set_xterm_title(title);
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
	if (client.IsConnected() &&
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

	screen_status_printf(_("Connecting to %s"),
			     client.GetSettingsName().c_str());
	doupdate();

	client.Connect();
}

void
mpdclient_connected_callback() noexcept
{
#ifndef NCMPC_MINI
	/* quit if mpd is pre 0.14 - song id not supported by mpd */
	auto &client = global_instance->GetClient();
	auto *connection = client.GetConnection();
	if (mpd_connection_cmp_server_version(connection, 0, 21, 0) < 0) {
		const unsigned *version =
			mpd_connection_get_server_version(connection);
		screen_status_printf(_("Error: MPD version %d.%d.%d is too old (%s needed)"),
				     version[0], version[1], version[2],
				     "0.21.0");
		client.Disconnect();
		doupdate();

		/* try again after 30 seconds */
		global_instance->ScheduleReconnect(std::chrono::seconds(30));
		return;
	}
#endif

	screen->status_bar.ClearMessage();
	doupdate();

	global_instance->UpdateClient();

	auto_update_timer(*global_instance);
}

void
mpdclient_failed_callback() noexcept
{
	/* try again in 5 seconds */
	global_instance->ScheduleReconnect(std::chrono::seconds(5));
}

void
mpdclient_lost_callback() noexcept
{
	screen->Update(global_instance->GetClient(),
		       global_instance->GetSeek());

	global_instance->ScheduleReconnect(std::chrono::seconds(1));
}

/**
 * This function is called by the gidle.c library when MPD sends us an
 * idle event (or when the connection dies).
 */
void
mpdclient_idle_callback([[maybe_unused]] unsigned events) noexcept
{
	auto &client = global_instance->GetClient();

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title(client);
#endif

	screen->Update(client, global_instance->GetSeek());
	auto_update_timer(*global_instance);
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
begin_input_event() noexcept
{
}

void end_input_event() noexcept
{
	auto &client = global_instance->GetClient();

	screen->Update(client, global_instance->GetSeek());
	client.events = (enum mpd_idle)0;

	auto_update_timer(*global_instance);
}

bool
do_input_event(EventLoop &event_loop, Command cmd) noexcept
{
	if (cmd == Command::QUIT) {
		event_loop.Break();
		return false;
	}

	try {
		screen->OnCommand(global_instance->GetClient(),
				  global_instance->GetSeek(), cmd);
	} catch (...) {
		screen_status_error(std::current_exception());
		return true;
	}

	if (cmd == Command::VOLUME_UP || cmd == Command::VOLUME_DOWN)
		/* make sure we don't update the volume yet */
		global_instance->DisableUpdateTimer();

	return true;
}

#ifdef HAVE_GETMOUSE

void
do_mouse_event(Point p, mmask_t bstate) noexcept
{
	try {
		screen->OnMouse(global_instance->GetClient(),
				global_instance->GetSeek(), p, bstate);
	} catch (...) {
		screen_status_error(std::current_exception());
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

	screen_status_message(buf);

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

	/* parse command line options - 2 pass */
	options_parse(argc, argv);

	const ScopeCursesInit curses_init;

	/* create the global Instance */
	Instance instance;
	global_instance = &instance;
	screen = &instance.GetScreenManager();

	AtScopeExit() {
		/* this must be executed after ~Instance(), so we're
		   using AtScopeExit() to do the trick */
#ifndef NCMPC_MINI
		set_xterm_title("");
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
