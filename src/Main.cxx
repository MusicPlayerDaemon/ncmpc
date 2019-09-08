/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2019 The Music Player Daemon Project
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
#include "util/ScopeExit.hxx"
#include "util/StringUTF8.hxx"
#include "util/Compiler.h"

#ifndef NCMPC_MINI
#include "conf.hxx"
#endif

#ifdef ENABLE_LYRICS_SCREEN
#include "lyrics.hxx"
#endif

#include <mpd/client.h>

#include <curses.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef ENABLE_LOCALE
#include <locale.h>
#endif

#define BUFSIZE 1024

static Instance *global_instance;
static struct mpdclient *mpd = nullptr;

ScreenManager *screen;

#ifndef NCMPC_MINI
static void
update_xterm_title()
{
	const struct mpd_song *song = mpd->GetPlayingSong();

	char tmp[BUFSIZE];
	const char *new_title = nullptr;
	if (!options.xterm_title_format.empty() && song != nullptr)
		new_title = strfsong(tmp, BUFSIZE,
				     options.xterm_title_format.c_str(), song) > 0
			? tmp
			: nullptr;

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
should_enable_update_timer()
{
	return mpd->playing;
}

static void
auto_update_timer()
{
	if (should_enable_update_timer())
		global_instance->EnableUpdateTimer();
	else
		global_instance->DisableUpdateTimer();
}

void
Instance::UpdateClient() noexcept
{
	if (client.IsConnected() &&
	    (client.events != 0 || client.playing))
		client.Update();

#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen_manager.Update(client, seek);
	client.events = (enum mpd_idle)0;
}

void
Instance::OnReconnectTimer(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

	assert(client.IsDead());

	screen_status_printf(_("Connecting to %s"),
			     client.GetSettingsName().c_str());
	doupdate();

	client.Connect();
}

void
mpdclient_connected_callback()
{
#ifndef NCMPC_MINI
	/* quit if mpd is pre 0.14 - song id not supported by mpd */
	auto *connection = mpd->GetConnection();
	if (mpd_connection_cmp_server_version(connection, 0, 19, 0) < 0) {
		const unsigned *version =
			mpd_connection_get_server_version(connection);
		screen_status_printf(_("Error: MPD version %d.%d.%d is too old (%s needed)"),
				     version[0], version[1], version[2],
				     "0.19.0");
		mpd->Disconnect();
		doupdate();

		/* try again after 30 seconds */
		global_instance->ScheduleReconnect(std::chrono::seconds(30));
		return;
	}
#endif

	screen->status_bar.ClearMessage();
	doupdate();

	global_instance->UpdateClient();

	auto_update_timer();
}

void
mpdclient_failed_callback()
{
	/* try again in 5 seconds */
	global_instance->ScheduleReconnect(std::chrono::seconds(5));
}

void
mpdclient_lost_callback()
{
	screen->Update(*mpd, global_instance->GetSeek());

	global_instance->ScheduleReconnect(std::chrono::seconds(1));
}

/**
 * This function is called by the gidle.c library when MPD sends us an
 * idle event (or when the connection dies).
 */
void
mpdclient_idle_callback(gcc_unused unsigned events)
{
#ifndef NCMPC_MINI
	if (options.enable_xterm_title)
		update_xterm_title();
#endif

	screen->Update(*mpd, global_instance->GetSeek());
	auto_update_timer();
}

void
Instance::OnUpdateTimer(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

	assert(pending_update_timer);
	pending_update_timer = false;

	UpdateClient();

	if (should_enable_update_timer())
		ScheduleUpdateTimer();
}

void begin_input_event()
{
}

void end_input_event()
{
	screen->Update(*mpd, global_instance->GetSeek());
	mpd->events = (enum mpd_idle)0;

	auto_update_timer();
}

bool
do_input_event(boost::asio::io_service &io_service, Command cmd)
{
	if (cmd == Command::QUIT) {
		io_service.stop();
		return false;
	}

	screen->OnCommand(*mpd, global_instance->GetSeek(), cmd);

	if (cmd == Command::VOLUME_UP || cmd == Command::VOLUME_DOWN)
		/* make sure we don't update the volume yet */
		global_instance->DisableUpdateTimer();

	return true;
}

#ifdef HAVE_GETMOUSE

void
do_mouse_event(Point p, mmask_t bstate)
{
	screen->OnMouse(*mpd, global_instance->GetSeek(), p, bstate);
}

#endif

#ifndef NCMPC_MINI
/**
 * Check the configured key bindings for errors, and display a status
 * message every 10 seconds.
 */
void
Instance::OnCheckKeyBindings(const boost::system::error_code &error) noexcept
{
	if (error)
		return;

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
{
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

	const ScopeInitUTF8 init_utf8;

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

#ifdef ENABLE_LYRICS_SCREEN
	lyrics_init();
#endif

	/* create the global Instance */
	Instance instance;
	global_instance = &instance;
	mpd = &instance.GetClient();
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

	auto_update_timer();

#ifndef NCMPC_MINI
	instance.ScheduleCheckKeyBindings();
#endif

	instance.Run();
	return 0;
}
