/* ncmpc (Ncurses MPD Client)
 * Copyright 2004-2021 The Music Player Daemon Project
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

#include "Instance.hxx"
#include "Options.hxx"
#include "event/SignalMonitor.hxx"

#include <signal.h>

#ifdef HAVE_TAG_WHITELIST

#include "strfsong.hxx"
#include "TagMask.hxx"

static constexpr TagMask global_tag_whitelist{
	/* these tags are used by SongPage.cxx */
	MPD_TAG_ARTIST,
	MPD_TAG_TITLE,
	MPD_TAG_ALBUM,
	MPD_TAG_COMPOSER,
	MPD_TAG_PERFORMER,
#if LIBMPDCLIENT_CHECK_VERSION(2,17,0)
	MPD_TAG_CONDUCTOR,
	MPD_TAG_WORK,
#endif
	MPD_TAG_NAME,
	MPD_TAG_DISC,
	MPD_TAG_TRACK,
	MPD_TAG_DATE,
	MPD_TAG_GENRE,
	MPD_TAG_COMMENT,
};

#endif

Instance::Instance()
	:client(event_loop,
		options.host.empty() ? nullptr : options.host.c_str(),
		options.port,
		options.timeout_ms,
		options.password.empty() ? nullptr : options.password.c_str()),
	 seek(event_loop, client),
	 reconnect_timer(event_loop, BIND_THIS_METHOD(OnReconnectTimer)),
	 update_timer(event_loop, BIND_THIS_METHOD(OnUpdateTimer)),
#ifndef NCMPC_MINI
	 check_key_bindings_timer(event_loop, BIND_THIS_METHOD(OnCheckKeyBindings)),
#endif
	 screen_manager(event_loop),
#ifdef ENABLE_LIRC
	 lirc_input(event_loop),
#endif
	 user_input(event_loop, *screen_manager.main_window.w)
{
	screen_manager.Init(&client);

#ifndef _WIN32
	SignalMonitorInit(event_loop);
	SignalMonitorRegister(SIGTERM, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGINT, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGHUP, BIND_THIS_METHOD(Quit));
	SignalMonitorRegister(SIGWINCH, BIND_THIS_METHOD(OnSigwinch));
	SignalMonitorRegister(SIGCONT, BIND_THIS_METHOD(OnSigwinch));
#endif

#ifdef HAVE_TAG_WHITELIST
	TagMask tag_mask = global_tag_whitelist;
	tag_mask |= SongFormatToTagMask(options.list_format.c_str());
	tag_mask |= SongFormatToTagMask(options.search_format.c_str());
	tag_mask |= SongFormatToTagMask(options.status_format.c_str());
#ifndef NCMPC_MINI
	tag_mask |= SongFormatToTagMask(options.xterm_title_format.c_str());
#endif

	client.WhitelistTags(tag_mask);
#endif
}

Instance::~Instance()
{
	screen_manager.Exit();
	SignalMonitorFinish();
}

void
Instance::Run()
{
	screen_manager.Update(client, seek);

	event_loop.Run();
}
